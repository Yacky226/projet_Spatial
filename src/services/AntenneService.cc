#include "AntenneService.h"
#include "../utils/ErrorHandler.h"

#include <json/json.h>
#include <optional>
#include <string>

using namespace drogon;
using namespace drogon::orm;// ============================================================================
// NOUVEAU : CLUSTERED ANTENNAS (Sprint 1 - Backend Clustering Optimization)
// ============================================================================
// NOUVEAU : CLUSTERED ANTENNAS (Sprint 1 - Backend Clustering Optimization)
// ============================================================================
/**
 * Clustering backend utilisant ST_SnapToGrid pour grouper les antennes proches
 * 
 * Principe:
 * 1. ST_SnapToGrid arrondit les coordonnées sur une grille, regroupant les points proches
 * 2. La taille de grille diminue avec le zoom (plus de détails = grille plus fine)
 * 3. Les clusters (count > 1) retournent un centroïde + metadata (count, ids)
 * 4. Les antennes seules (count = 1) retournent leurs données complètes
 * 
 * Calcul de la taille de grille:
 * - Zoom 0-5 (monde/continents): 1.0° (~111 km)
 * - Zoom 6-8 (pays): 0.5° (~55 km)
 * - Zoom 9-11 (régions): 0.1° (~11 km)
 * - Zoom 12-14 (villes): 0.01° (~1.1 km)
 * - Zoom 15-18 (quartiers): 0.001° (~111 m)
 */
void AntenneService::getClusteredAntennas(
    double minLat, double minLon, double maxLat, double maxLon,
    int zoom, const std::string& status, const std::string& technology, int operator_id,
    std::function<void(const Json::Value&, const std::string&)> callback) 
{
    auto client = app().getDbClient();
    
    // ========== CALCUL DE LA TAILLE DE GRILLE SELON LE ZOOM ==========
    // Plus le zoom est élevé, plus la grille est fine (plus de détails)
    double gridSize;
    if (zoom <= 5) {
        gridSize = 1.0;      // Zoom monde/continents: ~111 km
    } else if (zoom <= 8) {
        gridSize = 0.5;      // Zoom pays: ~55 km
    } else if (zoom <= 11) {
        gridSize = 0.1;      // Zoom régions: ~11 km
    } else if (zoom <= 14) {
        gridSize = 0.01;     // Zoom villes: ~1.1 km
    } else {
        gridSize = 0.001;    // Zoom quartiers: ~111 m
    }
    
    // ========== CONSTRUCTION DE LA CLAUSE WHERE POUR LES FILTRES ==========
    std::vector<std::string> whereClauses;
    whereClauses.push_back("geom && ST_MakeEnvelope($1, $2, $3, $4, 4326)");
    
    int paramIndex = 5; // Les 4 premiers params sont minLon, minLat, maxLon, maxLat
    std::string statusParam, techParam;
    int operatorParam = 0;
    
    if (!status.empty()) {
        whereClauses.push_back("status = $" + std::to_string(paramIndex++) + "::antenna_status");
        statusParam = status;
    }
    if (!technology.empty()) {
        whereClauses.push_back("technology = $" + std::to_string(paramIndex++) + "::technology_type");
        techParam = technology;
    }
    if (operator_id > 0) {
        whereClauses.push_back("operator_id = $" + std::to_string(paramIndex++));
        operatorParam = operator_id;
    }
    
    std::string whereClause;
    for (size_t i = 0; i < whereClauses.size(); ++i) {
        if (i > 0) whereClause += " AND ";
        whereClause += whereClauses[i];
    }
    
    // ========== REQUÊTE SQL AVEC CLUSTERING ==========
    // Utilise ST_SnapToGrid pour regrouper les points proches
    // GROUP BY sur la grille pour compter les antennes par cellule
    std::string sql = R"(
        WITH snapped AS (
            SELECT 
                id,
                coverage_radius,
                status,
                technology,
                installation_date,
                operator_id,
                geom,
                ST_SnapToGrid(geom, )" + std::to_string(gridSize) + R"() AS grid_point
            FROM antenna
            WHERE )" + whereClause + R"(
        ),
        clusters AS (
            SELECT 
                grid_point,
                COUNT(*) as point_count,
                ARRAY_AGG(id) as antenna_ids,
                ST_AsGeoJSON(ST_Centroid(ST_Collect(geom)))::json as centroid_geojson,
                -- Pour les clusters, on agrège les métadonnées
                ARRAY_AGG(status::text) as statuses,
                ARRAY_AGG(technology::text) as technologies,
                ARRAY_AGG(operator_id) as operator_ids,
                AVG(coverage_radius) as avg_radius
            FROM snapped
            GROUP BY grid_point
        )
        SELECT json_build_object(
            'type', 'FeatureCollection',
            'features', COALESCE(json_agg(
                json_build_object(
                    'type', 'Feature',
                    'geometry', centroid_geojson,
                    'properties', json_build_object(
                        'cluster', CASE WHEN point_count > 1 THEN true ELSE false END,
                        'point_count', point_count,
                        'antenna_ids', antenna_ids,
                        'avg_radius', ROUND(avg_radius::numeric, 2),
                        'statuses', statuses,
                        'technologies', technologies,
                        'operator_ids', operator_ids
                    )
                )
            ), '[]'::json)
        ) as geojson
        FROM clusters
    )";
    
    // ========== EXÉCUTION AVEC PARAMÈTRES DYNAMIQUES ==========
    // Construction des arguments pour execSqlAsync
    // Note: L'ordre des paramètres doit correspondre aux $1, $2, etc. dans le SQL
    auto executeQuery = [&](auto&&... args) {
        client->execSqlAsync(
            sql,
            [callback, zoom, gridSize](const Result& r) {
                if (!r.empty() && !r[0]["geojson"].isNull()) {
                    std::string geojsonStr = r[0]["geojson"].as<std::string>();
                    Json::Value geojson;
                    Json::Reader reader;
                    
                    if (reader.parse(geojsonStr, geojson)) {
                        // Ajout de métadonnées pour debug/monitoring
                        if (!geojson.isMember("metadata")) {
                            geojson["metadata"] = Json::Value(Json::objectValue);
                        }
                        geojson["metadata"]["cluster_method"] = "ST_SnapToGrid";
                        geojson["metadata"]["grid_size"] = gridSize;
                        geojson["metadata"]["zoom_level"] = zoom;
                        
                        int totalFeatures = geojson["features"].size();
                        int clusterCount = 0;
                        int singleCount = 0;
                        
                        for (const auto& feature : geojson["features"]) {
                            if (feature["properties"]["cluster"].asBool()) {
                                clusterCount++;
                            } else {
                                singleCount++;
                            }
                        }
                        
                        geojson["metadata"]["total_features"] = totalFeatures;
                        geojson["metadata"]["clusters"] = clusterCount;
                        geojson["metadata"]["singles"] = singleCount;
                        
                        LOG_INFO << "Clustering: " << totalFeatures << " features (" 
                                 << clusterCount << " clusters, " << singleCount 
                                 << " singles) at zoom " << zoom << ", grid " << gridSize;
                        
                        callback(geojson, "");
                    } else {
                        auto errorDetails = ErrorHandler::analyzePostgresError("Failed to parse GeoJSON response");
                        ErrorHandler::logError("AntenneService::getClusteredAntennas", errorDetails);
                        callback(Json::Value(), errorDetails.userMessage);
                    }
                } else {
                    // Pas de résultats, retourner FeatureCollection vide
                    Json::Value emptyCollection;
                    emptyCollection["type"] = "FeatureCollection";
                    emptyCollection["features"] = Json::Value(Json::arrayValue);
                    emptyCollection["metadata"] = Json::Value(Json::objectValue);
                    emptyCollection["metadata"]["total_features"] = 0;
                    callback(emptyCollection, "");
                }
            },
            [callback](const DrogonDbException& e) {
                auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                ErrorHandler::logError("AntenneService::getClusteredAntennas", errorDetails);
                callback(Json::Value(), errorDetails.userMessage);
            },
            std::forward<decltype(args)>(args)...
        );
    };
    
    // ========== APPEL AVEC LES BONS PARAMÈTRES SELON LES FILTRES ==========
    // On doit passer les paramètres dans l'ordre: bbox, puis filtres optionnels
    if (!status.empty() && !technology.empty() && operator_id > 0) {
        executeQuery(minLon, minLat, maxLon, maxLat, statusParam, techParam, operatorParam);
    } else if (!status.empty() && !technology.empty()) {
        executeQuery(minLon, minLat, maxLon, maxLat, statusParam, techParam);
    } else if (!status.empty() && operator_id > 0) {
        executeQuery(minLon, minLat, maxLon, maxLat, statusParam, operatorParam);
    } else if (!technology.empty() && operator_id > 0) {
        executeQuery(minLon, minLat, maxLon, maxLat, techParam, operatorParam);
    } else if (!status.empty()) {
        executeQuery(minLon, minLat, maxLon, maxLat, statusParam);
    } else if (!technology.empty()) {
        executeQuery(minLon, minLat, maxLon, maxLat, techParam);
    } else if (operator_id > 0) {
        executeQuery(minLon, minLat, maxLon, maxLat, operatorParam);
    } else {
        executeQuery(minLon, minLat, maxLon, maxLat);
    }
}

// ============================================================================
// 12. GET SIMPLIFIED COVERAGE (Ultra-optimisé + Filtres operator/technology)
// ============================================================================
void AntenneService::getSimplifiedCoverage(
    double minLat, double minLon, double maxLat, double maxLon, int zoom,
    int operator_id, const std::string& technology,
    std::function<void(const Json::Value&, const std::string&)> callback)
{
    auto client = app().getDbClient();
    
    // Calcul du seuil de simplification basé sur le zoom
    // Zoom bas = simplification agressive, zoom haut = plus de détails
    double tolerance;
    if (zoom <= 6) {
        tolerance = 0.05;  // ~5.5 km - Simplification maximale
    } else if (zoom <= 8) {
        tolerance = 0.02;  // ~2.2 km
    } else if (zoom <= 10) {
        tolerance = 0.01;  // ~1.1 km
    } else if (zoom <= 12) {
        tolerance = 0.005; // ~550 m
    } else {
        tolerance = 0.001; // ~111 m - Détails fins
    }
    
    // Construction des clauses WHERE pour les filtres
    std::vector<std::string> whereClauses;
    whereClauses.push_back("status = 'active'");
    whereClauses.push_back("ST_Intersects(geom, ST_MakeEnvelope($1, $2, $3, $4, 4326))");
    
    int paramIndex = 7; // $1-$6 déjà utilisés
    std::string operatorFilter, techFilter;
    
    if (operator_id > 0) {
        whereClauses.push_back("operator_id = $" + std::to_string(paramIndex++));
        operatorFilter = std::to_string(operator_id);
    }
    if (!technology.empty()) {
        whereClauses.push_back("technology = $" + std::to_string(paramIndex++) + "::technology_type");
        techFilter = technology;
    }
    
    std::string whereClause = whereClauses[0];
    for (size_t i = 1; i < whereClauses.size(); i++) {
        whereClause += " AND " + whereClauses[i];
    }
    
    // Stratégie d'optimisation :
    // 1. ST_Buffer crée les cercles de couverture
    // 2. ST_Union fusionne tous les cercles en 1 seule géométrie
    // 3. ST_Simplify réduit drastiquement les points (Douglas-Peucker)
    // 4. ST_MakeValid corrige les auto-intersections éventuelles
    // 5. Filtre par bbox pour limiter la zone calculée
    // 6. Filtres optionnels operator_id et technology
    
    std::string sql = R"(
        WITH coverage_raw AS (
            SELECT ST_Union(
                ST_Buffer(geom::geography, coverage_radius)::geometry
            ) as geom
            FROM antenna
            WHERE )" + whereClause + R"(
        ),
        coverage_simplified AS (
            SELECT ST_MakeValid(
                ST_Simplify(geom, $5)
            ) as geom
            FROM coverage_raw
            WHERE geom IS NOT NULL
        )
        SELECT 
            json_build_object(
                'type', 'FeatureCollection',
                'features', COALESCE(
                    json_agg(
                        json_build_object(
                            'type', 'Feature',
                            'geometry', ST_AsGeoJSON(geom)::json,
                            'properties', json_build_object(
                                'type', 'coverage',
                                'zoom', $6::integer,
                                'tolerance', $5::double precision
                            )
                        )
                    ),
                    '[]'::json
                ),
                'metadata', json_build_object(
                    'zoom', $6::integer,
                    'simplification_tolerance', $5::double precision,
                    'bbox', json_build_object(
                        'minLon', $1, 'minLat', $2,
                        'maxLon', $3, 'maxLat', $4
                    )
                )
            ) as geojson
        FROM coverage_simplified
    )";
    
    // Exécution avec les bons paramètres selon les filtres
    auto executeQuery = [&](auto&&... params) {
        client->execSqlAsync(sql,
            [callback, zoom](const Result &r) {
                if (!r.empty() && !r[0]["geojson"].isNull()) {
                    std::string jsonStr = r[0]["geojson"].as<std::string>();
                    Json::Value result;
                    Json::CharReaderBuilder builder;
                    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                    std::string errs;
                    
                    if (reader->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.length(), &result, &errs)) {
                        LOG_INFO << "✅ Simplified coverage generated for zoom " << zoom;
                        callback(result, "");
                    } else {
                        auto errorDetails = ErrorHandler::analyzePostgresError("Failed to parse coverage GeoJSON");
                        ErrorHandler::logError("AntenneService::getSimplifiedCoverage", errorDetails);
                        callback(Json::Value(), errorDetails.userMessage);
                    }
                } else {
                    // Pas de couverture dans cette bbox
                    Json::Value emptyCollection;
                    emptyCollection["type"] = "FeatureCollection";
                    emptyCollection["features"] = Json::Value(Json::arrayValue);
                    emptyCollection["metadata"]["zoom"] = zoom;
                    callback(emptyCollection, "");
                }
            },
            [callback](const DrogonDbException& e) {
                auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                ErrorHandler::logError("AntenneService::getSimplifiedCoverage", errorDetails);
                callback(Json::Value(), errorDetails.userMessage);
            },
            std::forward<decltype(params)>(params)...
        );
    };
    
    // Appeler avec les bons paramètres selon filtres
    if (operator_id > 0 && !technology.empty()) {
        executeQuery(minLon, minLat, maxLon, maxLat, tolerance, zoom, operator_id, technology);
    } else if (operator_id > 0) {
        executeQuery(minLon, minLat, maxLon, maxLat, tolerance, zoom, operator_id);
    } else if (!technology.empty()) {
        executeQuery(minLon, minLat, maxLon, maxLat, tolerance, zoom, technology);
    } else {
        executeQuery(minLon, minLat, maxLon, maxLat, tolerance, zoom);
    }
}