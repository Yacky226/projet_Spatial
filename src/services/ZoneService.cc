#include "ZoneService.h"
using namespace drogon;
using namespace drogon::orm;

// 1. CREATE
void ZoneService::create(const ZoneModel &z, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    
    // Gestion du parent_id (NULL si 0)
    std::string sql = "INSERT INTO zone (name, type, density, geom, parent_id) "
                      "VALUES ($1, $2::zone_type, $3, ST_GeomFromText($4, 4326), $5)";

    // Astuce : Si parent_id est 0, on envoie nullptr pour que SQL mette NULL
    // Mais Drogon gère mal nullptr direct, on envoie NULL via conditionnel SQL ou on laisse 0 si la FK l'autorise. 
    // Ici on suppose que la DB accepte NULL. Pour simplifier en C++ sans std::optional, on envoie z.parent_id si > 0, sinon NULL est géré par la logique SQL.
    // Pour faire simple ici : on utilise une requête dynamique ou on accepte NULL.
    // Note: Pour simplifier ce code exemple, on insère NULL si parent_id == 0 via SQL CASE ou similaire est complexe.
    // On va assumer que l'utilisateur ne met pas de parent_id pour l'instant ou envoie un ID valide.
    
    client->execSqlAsync(sql, 
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        z.name, z.type, z.density, z.wkt_geometry, (z.parent_id > 0 ? std::make_optional(z.parent_id) : std::nullopt)
    );
}

// 2. READ ALL
void ZoneService::getAll(std::function<void(const std::vector<ZoneModel>&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, name, type, density, parent_id, ST_AsText(geom) as wkt FROM zone ORDER BY id";

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
    }, [callback](const DrogonDbException &e) { callback({}, e.base().what()); });
}

// 3. READ ONE
void ZoneService::getById(int id, std::function<void(const ZoneModel&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, name, type, density, parent_id, ST_AsText(geom) as wkt FROM zone WHERE id = $1";

    client->execSqlAsync(sql, [callback](const Result &r) {
        if (r.size() == 0) { callback({}, "Not Found"); return; }
        auto row = r[0];
        ZoneModel z;
        z.id = row["id"].as<int>();
        z.name = row["name"].as<std::string>();
        z.type = row["type"].as<std::string>();
        z.density = row["density"].as<double>();
        z.parent_id = row["parent_id"].isNull() ? 0 : row["parent_id"].as<int>();
        z.wkt_geometry = row["wkt"].as<std::string>();
        callback(z, "");
    }, [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, id);
}

// 3.5. READ BY TYPE
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

// 4. UPDATE
void ZoneService::update(const ZoneModel &z, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    // Update complet avec géométrie
    std::string sql = "UPDATE zone SET name=$1, type=$2::zone_type, density=$3, "
                      "geom=ST_GeomFromText($4, 4326), parent_id=$5 WHERE id=$6";

    client->execSqlAsync(sql, 
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        z.name, z.type, z.density, z.wkt_geometry, (z.parent_id > 0 ? std::make_optional(z.parent_id) : std::nullopt), z.id
    );
}

// 5. DELETE
void ZoneService::remove(int id, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("DELETE FROM zone WHERE id = $1",
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        id);
}

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
// 7. GET WHITE ZONES (Zone Geom - Operator Coverage)
// ============================================================================
void ZoneService::getWhiteZones(int zone_id, int operator_id, 
                               std::function<void(const Json::Value&, const std::string&)> callback) {
    auto client = app().getDbClient();
    
    // Logique SQL :
    // 1. "coverage": Calcule l'union de toutes les zones de couverture de l'opérateur (buffers).
    // 2. ST_Difference: Soustrait cette couverture de la géométrie de la zone cible.
    // 3. COALESCE: Si la couverture est vide (aucune antenne), on soustrait une géométrie vide (donc on garde toute la zone).
    
    std::string sql = R"(
        WITH coverage AS (
            SELECT ST_Union(ST_Buffer(geom::geography, coverage_radius)::geometry) as geom
            FROM antenna
            WHERE operator_id = $2
        )
        SELECT ST_AsGeoJSON(
            ST_Difference(
                z.geom::geometry, 
                COALESCE((SELECT geom FROM coverage), 'GEOMETRYCOLLECTION EMPTY'::geometry)
            )
        ) as geojson
        FROM zone z
        WHERE z.id = $1
    )";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            Json::Value featureCollection;
            featureCollection["type"] = "FeatureCollection";
            Json::Value features(Json::arrayValue);
            
            if (r.size() > 0 && !r[0]["geojson"].isNull()) {
                Json::Value feature;
                feature["type"] = "Feature";
                feature["properties"] = Json::Value(Json::objectValue);
                feature["properties"]["type"] = "white_zone"; // Marqueur pour le frontend
                
                Json::CharReaderBuilder builder;
                std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                std::string errs;
                std::string geojsonString = r[0]["geojson"].as<std::string>();
                Json::Value geometryObject;
                
                if (reader->parse(geojsonString.c_str(), geojsonString.c_str() + geojsonString.length(), &geometryObject, &errs)) {
                    feature["geometry"] = geometryObject;
                    features.append(feature);
                }
            }
            
            featureCollection["features"] = features;
            callback(featureCollection, "");
        },
        [callback](const DrogonDbException &e) {
            Json::Value empty;
            // On log l'erreur mais on renvoie un GeoJSON vide pour ne pas casser le front
            LOG_ERROR << "Error calculating white zones: " << e.base().what();
            callback(empty, e.base().what());
        }, 
        zone_id, operator_id
    );
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