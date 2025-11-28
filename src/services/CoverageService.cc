#include "CoverageService.h"
#include "../utils/ErrorHandler.h"
#include <cmath> 

using namespace drogon;
using namespace drogon::orm;

void CoverageService::calculateForAntenna(int antennaId, 
                                          std::function<void(const CoverageResult&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // REQUÊTE COMPLEXE POSTGIS
    // 1. On crée un buffer (cercle) autour du point de l'antenne
    // 2. On intersecte ce cercle avec les zones
    // 3. On récupère la densité de la zone pour estimer la population
    
    std::string sql = R"(
        WITH antenna_cov AS (
            SELECT 
                id, 
                ST_Buffer(geom::geography, coverage_radius)::geometry as geom 
            FROM antenna 
            WHERE id = $1
        )
        SELECT 
            z.name as zone_name,
            z.density,
            ST_Area(ST_Intersection(z.geom, ac.geom)::geography) / 1000000.0 as area_km2
        FROM zone z, antenna_cov ac
        WHERE ST_Intersects(z.geom, ac.geom)
    )";

    client->execSqlAsync(sql,
        [callback, antennaId](const Result &r) {
            CoverageResult result;
            result.antenna_id = antennaId;
            result.total_population_covered = 0;
            result.total_area_km2 = 0;

            for (auto row : r) {
                ZoneCoverageDetail detail;
                detail.zone_name = row["zone_name"].as<std::string>();
                detail.intersection_area_km2 = row["area_km2"].as<double>();
                
                // Calcul : Surface (km²) * Densité (hab/km²)
                double density = row["density"].as<double>();
                detail.covered_population = detail.intersection_area_km2 * density;

                // Agrégation
                result.total_area_km2 += detail.intersection_area_km2;
                result.total_population_covered += detail.covered_population;
                
                // Ajout au vecteur de détails
                result.details.push_back(detail);
            }
            
            // Arrondi à l'entier pour la population totale
            result.total_population_covered = std::round(result.total_population_covered);

            callback(result, "");
        },
        [callback](const DrogonDbException &e) {
            callback({}, e.base().what());
        },
        antennaId
    );
}

// ============================================================================
// DÉTECTION DES ZONES BLANCHES
// ============================================================================
void CoverageService::getWhiteZones(
    int zone_id,
    std::function<void(const Json::Value&, const std::string&)> callback) {
    
    auto client = app().getDbClient();
    
    std::string sql = R"(
        WITH target_zone AS (
            SELECT id, geom, name
            FROM zone
            WHERE id = $1
        ),
        all_coverage AS (
            SELECT ST_Union(
                ST_Buffer(a.geom::geography, a.coverage_radius)::geometry
            ) as covered_area
            FROM antenna a, target_zone z
            WHERE a.status = 'active'
              AND ST_DWithin(a.geom::geography, z.geom::geography, a.coverage_radius)
        ),
        white_zones_geom AS (
            SELECT 
                z.name,
                z.id,
                ST_Difference(z.geom, COALESCE(c.covered_area, 'GEOMETRYCOLLECTION EMPTY'::geometry)) as uncovered,
                ST_Area(z.geom::geography) / 1000000.0 as total_area_km2
            FROM target_zone z
            LEFT JOIN all_coverage c ON true
        ),
        white_zones_stats AS (
            SELECT 
                name,
                id,
                uncovered,
                total_area_km2,
                ST_Area(uncovered::geography) / 1000000.0 as uncovered_area_km2,
                (ST_Area(uncovered::geography) / ST_Area(ST_GeomFromText(ST_AsText(uncovered), 4326)::geography)) * 100 as coverage_gap_percent
            FROM white_zones_geom
            WHERE NOT ST_IsEmpty(uncovered)
        )
        SELECT json_build_object(
            'type', 'FeatureCollection',
            'metadata', json_build_object(
                'zone_id', id,
                'zone_name', name,
                'total_area_km2', total_area_km2,
                'uncovered_area_km2', uncovered_area_km2,
                'coverage_gap_percent', coverage_gap_percent
            ),
            'features', json_agg(
                json_build_object(
                    'type', 'Feature',
                    'geometry', ST_AsGeoJSON(
                        CASE 
                            WHEN ST_GeometryType(uncovered) = 'ST_GeometryCollection' 
                            THEN (ST_Dump(uncovered)).geom
                            ELSE uncovered
                        END
                    )::json,
                    'properties', json_build_object(
                        'type', 'white_zone',
                        'area_km2', ST_Area(uncovered::geography) / 1000000.0
                    )
                )
            )
        ) as result
        FROM white_zones_stats
        GROUP BY id, name, total_area_km2, uncovered_area_km2, coverage_gap_percent
    )";
    
    client->execSqlAsync(sql,
        [callback](const Result& r) {
            if (!r.empty() && !r[0]["result"].isNull()) {
                std::string jsonStr = r[0]["result"].as<std::string>();
                Json::Value result;
                
                Json::CharReaderBuilder builder;
                std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                std::string errs;

                if (reader->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.length(), &result, &errs)) {
                    callback(result, "");
                } else {
                     callback(Json::Value(), "JSON parsing error: " + errs);
                }
            } else {
                Json::Value empty;
                empty["type"] = "FeatureCollection";
                empty["features"] = Json::arrayValue;
                empty["metadata"]["message"] = "No white zones found (100% coverage)";
                callback(empty, "");
            }
        },
        [callback](const DrogonDbException& e) {
            callback(Json::Value(), e.base().what());
        },
        zone_id
    );
}