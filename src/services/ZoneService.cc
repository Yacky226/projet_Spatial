#include "ZoneService.h"
using namespace drogon;
using namespace drogon::orm;

// Récupération des zones par type
void ZoneService::getByType(const std::string& type, std::function<void(const std::vector<ZoneModel>&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, name, type, density, parent_id, ST_AsText(geom) as wkt FROM zone WHERE type = $1::zone_type ORDER BY name";

    client->execSqlAsync(sql, [callback](const Result &r) {
        std::vector<ZoneModel> list;
        for (auto row : r) {
            ZoneModel z;
            z.id = row["id"].as<int>();
            z.name = row["name"].as<std::string>();
            z.type = row["type"].as<std::string>();
            z.density = row["density"].as<double>();
            z.parent_id = row["parent_id"].isNull() ? 0 : row["parent_id"].as<int>();
            z.wkt_geometry = row["wkt"].as<std::string>();
            list.push_back(z);
        }
        callback(list, "");
    }, [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, type);
}

// Récupération de toutes les zones en GeoJSON
void ZoneService::getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback) {
    auto client = app().getDbClient();
    
    // ST_AsGeoJSON fonctionne aussi parfaitement sur des POLYGONES
    std::string sql = "SELECT id, name, type, density, ST_AsGeoJSON(geom) as geojson FROM zone";

    client->execSqlAsync(sql, [callback](const Result &r) {
        Json::Value featureCollection;
        featureCollection["type"] = "FeatureCollection";
        Json::Value features(Json::arrayValue);
        
        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errs;

        for (auto row : r) {
            Json::Value feature;
            feature["type"] = "Feature";

            // Propriétés
            Json::Value props;
            props["id"] = row["id"].as<int>();
            props["name"] = row["name"].as<std::string>();
            props["density"] = row["density"].as<double>();
            feature["properties"] = props;

            // Géométrie
            std::string geoStr = row["geojson"].as<std::string>();
            Json::Value geomObj;
            if(reader->parse(geoStr.c_str(), geoStr.c_str() + geoStr.length(), &geomObj, &errs)){
                feature["geometry"] = geomObj;
            }
            features.append(feature);
        }
        featureCollection["features"] = features;
        callback(featureCollection, "");
    },
    [callback](const DrogonDbException &e) { 
        Json::Value empty; callback(empty, e.base().what()); 
    });
}



// ============================================================================
// NOUVEAU : SIMPLIFICATION GÉOMÉTRIQUE (Sprint 2 - Optimization)
// ============================================================================
/**
 * Récupère les zones par type avec simplification géométrique adaptative
 * 
 * Objectif: Réduire la complexité des polygones selon le niveau de zoom
 * Technique: Utilise ST_Simplify de PostGIS avec tolérance adaptative
 * 
 * Tolérance selon zoom:
 * - Zoom 0-6 (monde/continents): 0.01° (~1.1 km) - Très simplifié
 * - Zoom 7-10 (pays/régions): 0.005° (~550 m) - Moyennement simplifié
 * - Zoom 11-14 (villes): 0.001° (~111 m) - Peu simplifié
 * - Zoom 15+ (quartiers): 0.0001° (~11 m) - Détails préservés
 * 
 * @param type Type de zone (country, region, province, commune, etc.)
 * @param zoom Niveau de zoom Leaflet (0-18)
 * @param callback Retourne les zones avec géométries simplifiées ou erreur
 */
void ZoneService::getByTypeSimplified(
    const std::string& type, 
    int zoom,
    std::function<void(const std::vector<ZoneModel>&, const std::string&)> callback) 
{
    auto client = app().getDbClient();
    
    // Calculer la tolérance de simplification selon le zoom
    double tolerance = calculateSimplificationTolerance(zoom);
    
    // Requête SQL avec ST_Simplify
    // preserve_collapsed = true pour garder les petites géométries
    std::string sql = R"(
        SELECT 
            id, 
            name, 
            type, 
            density, 
            parent_id,
            ST_AsText(ST_Simplify(geom, $2, true)) as wkt
        FROM zone 
        WHERE type = $1::zone_type 
        ORDER BY name
    )";
    
    client->execSqlAsync(
        sql,
        [callback, tolerance, zoom](const Result& r) {
            std::vector<ZoneModel> list;
            for (auto row : r) {
                ZoneModel z;
                z.id = row["id"].as<int>();
                z.name = row["name"].as<std::string>();
                z.type = row["type"].as<std::string>();
                z.density = row["density"].as<double>();
                z.parent_id = row["parent_id"].isNull() ? 0 : row["parent_id"].as<int>();
                z.wkt_geometry = row["wkt"].as<std::string>();
                list.push_back(z);
            }
            
            LOG_INFO << "Fetched " << list.size() << " zones (type: " << (list.empty() ? "N/A" : list[0].type)
                     << ", zoom: " << zoom << ", tolerance: " << tolerance << ")";
            
            callback(list, "");
        },
        [callback](const DrogonDbException& e) {
            LOG_ERROR << "Error fetching simplified zones: " << e.base().what();
            callback({}, e.base().what());
        },
        type, 
        tolerance
    );
}

/**
 * Calcule la tolérance de simplification selon le niveau de zoom
 * 
 * Plus le zoom est élevé, plus la tolérance est faible (plus de détails)
 * Les valeurs sont en degrés (système de coordonnées 4326)
 * 
 * @param zoom Niveau de zoom Leaflet (0-18)
 * @return Tolérance en degrés pour ST_Simplify
 */
double ZoneService::calculateSimplificationTolerance(int zoom) {
    // Tolérance adaptative selon le zoom
    // Valeurs en degrés (SRID 4326)
    if (zoom <= 6) {
        return 0.01;      // ~1.1 km - Zoom monde/continents
    } else if (zoom <= 10) {
        return 0.005;     // ~550 m - Zoom pays/régions
    } else if (zoom <= 14) {
        return 0.001;     // ~111 m - Zoom villes
    } else {
        return 0.0001;    // ~11 m - Zoom quartiers (détails préservés)
    }
}

// ============================================================================
// NOUVEAU: RECHERCHE DE ZONES (Sprint - Optimization Modal)
// ============================================================================
/**
 * Recherche des zones par type et query string avec ILIKE (insensible à la casse)
 * 
 * Optimisations:
 * - ILIKE avec % pour recherche partielle
 * - LIMIT pour éviter trop de résultats
 * - Pas de géométrie complète (légère pour autocomplete)
 * - Index sur name pour performance
 * 
 * @param type Type de zone (country, region, province, commune)
 * @param query Texte de recherche (peut être vide pour tout récupérer)
 * @param limit Nombre max de résultats
 * @param callback Retourne les zones correspondantes ou erreur
 */
void ZoneService::searchZones(
    const std::string& type, 
    const std::string& query, 
    int limit,
    std::function<void(const std::vector<ZoneModel>&, const std::string&)> callback) 
{
    auto client = app().getDbClient();
    
    std::string sql;
    
    if (query.empty()) {
        // Si pas de query, retourner toutes les zones du type 
        sql = R"(
            SELECT 
                id, 
                name, 
                type, 
                density, 
                parent_id,
                ST_AsText(ST_Envelope(geom)) as bbox_wkt
            FROM zone 
            WHERE type = $1::zone_type 
            ORDER BY name
            LIMIT $2
        )";
        
        client->execSqlAsync(
            sql,
            [callback](const Result& r) {
                std::vector<ZoneModel> list;
                for (auto row : r) {
                    ZoneModel z;
                    z.id = row["id"].as<int>();
                    z.name = row["name"].as<std::string>();
                    z.type = row["type"].as<std::string>();
                    z.density = row["density"].as<double>();
                    z.parent_id = row["parent_id"].isNull() ? 0 : row["parent_id"].as<int>();
                    z.wkt_geometry = row["bbox_wkt"].as<std::string>(); // Envelope pour bounds
                    list.push_back(z);
                }
                callback(list, "");
            },
            [callback](const DrogonDbException& e) {
                LOG_ERROR << "Error searching zones: " << e.base().what();
                callback({}, e.base().what());
            },
            type, 
            std::to_string(limit)
        );
    } else {
        // Recherche avec ILIKE (insensible à la casse)
        sql = R"(
            SELECT 
                id, 
                name, 
                type, 
                density, 
                parent_id,
                ST_AsText(ST_Envelope(geom)) as bbox_wkt
            FROM zone 
            WHERE type = $1::zone_type 
              AND name ILIKE $2
            ORDER BY name
            LIMIT $3
        )";
        
        std::string searchPattern = "%" + query + "%";
        
        client->execSqlAsync(
            sql,
            [callback, query](const Result& r) {
                std::vector<ZoneModel> list;
                for (auto row : r) {
                    ZoneModel z;
                    z.id = row["id"].as<int>();
                    z.name = row["name"].as<std::string>();
                    z.type = row["type"].as<std::string>();
                    z.density = row["density"].as<double>();
                    z.parent_id = row["parent_id"].isNull() ? 0 : row["parent_id"].as<int>();
                    z.wkt_geometry = row["bbox_wkt"].as<std::string>(); // Envelope pour bounds
                    list.push_back(z);
                }
                
                LOG_INFO << "Found " << list.size() << " zones matching '" << query << "'";
                callback(list, "");
            },
            [callback](const DrogonDbException& e) {
                LOG_ERROR << "Error searching zones: " << e.base().what();
                callback({}, e.base().what());
            },
            type, 
            searchPattern,
            std::to_string(limit)
        );
    }
}