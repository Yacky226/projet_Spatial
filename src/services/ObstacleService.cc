#include "ObstacleService.h"
#include "../utils/ErrorHandler.h"
#include <json/json.h>
#include <string>

using namespace drogon;
using namespace drogon::orm;

// ============================================================================
// 1. CREATE
// ============================================================================
void ObstacleService::create(const ObstacleModel &obs, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    // Insertion : On stocke le type de géométrie et la géométrie elle-même via ST_GeomFromText
    std::string sql = "INSERT INTO obstacle (type, geom_type, geom) "
                      "VALUES ($1, $2, ST_GeomFromText($3, 4326)) RETURNING id";

    client->execSqlAsync(sql, 
        [callback](const Result &r) { 
            int newId = r[0]["id"].as<int>();
            LOG_INFO << "Obstacle created successfully with ID: " << newId;
            callback(""); 
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("ObstacleService::create", errorDetails);
            callback(errorDetails.userMessage);
        },
        obs.type, obs.geom_type, obs.wkt_geometry
    );
}

// ============================================================================
// 2. READ ALL (Non paginé)
// ============================================================================
void ObstacleService::getAll(std::function<void(const std::vector<ObstacleModel>&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, type, geom_type, ST_AsText(geom) as wkt FROM obstacle ORDER BY id";

    client->execSqlAsync(sql, 
        [callback](const Result &r) {
            std::vector<ObstacleModel> list;
            for (auto row : r) {
                ObstacleModel o;
                o.id = row["id"].as<int>();
                o.type = row["type"].as<std::string>();
                o.geom_type = row["geom_type"].as<std::string>();
                o.wkt_geometry = row["wkt"].as<std::string>();
                list.push_back(o);
            }
            LOG_INFO << "Retrieved " << list.size() << " obstacles";
            callback(list, "");
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("ObstacleService::getAll", errorDetails);
            callback({}, errorDetails.userMessage); 
        }
    );
}

// ============================================================================
// 3. READ ONE
// ============================================================================
void ObstacleService::getById(int id, std::function<void(const ObstacleModel&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, type, geom_type, ST_AsText(geom) as wkt FROM obstacle WHERE id = $1";

    client->execSqlAsync(sql, 
        [callback, id](const Result &r) {
            if (r.size() == 0) { 
                LOG_WARN << "Obstacle ID " << id << " not found";
                callback({}, "Not Found"); 
                return; 
            }
            auto row = r[0];
            ObstacleModel o;
            o.id = row["id"].as<int>();
            o.type = row["type"].as<std::string>();
            o.geom_type = row["geom_type"].as<std::string>();
            o.wkt_geometry = row["wkt"].as<std::string>();
            callback(o, "");
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("ObstacleService::getById", errorDetails);
            callback({}, errorDetails.userMessage); 
        }, 
        id
    );
}

// ============================================================================
// 4. UPDATE
// ============================================================================
void ObstacleService::update(const ObstacleModel &obs, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "UPDATE obstacle SET type=$1, geom_type=$2, geom=ST_GeomFromText($3, 4326) WHERE id=$4";

    client->execSqlAsync(sql, 
        [callback, obs](const Result &r) { 
            if (r.affectedRows() == 0) {
                LOG_WARN << "Update failed: Obstacle ID " << obs.id << " not found";
                callback("Not Found");
            } else {
                LOG_INFO << "Obstacle ID " << obs.id << " updated";
                callback(""); 
            }
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("ObstacleService::update", errorDetails);
            callback(errorDetails.userMessage); 
        },
        obs.type, obs.geom_type, obs.wkt_geometry, obs.id
    );
}

// ============================================================================
// 5. DELETE
// ============================================================================
void ObstacleService::remove(int id, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("DELETE FROM obstacle WHERE id = $1",
        [callback, id](const Result &r) { 
            if (r.affectedRows() == 0) {
                callback("Not Found");
            } else {
                LOG_INFO << "Obstacle ID " << id << " deleted";
                callback(""); 
            }
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("ObstacleService::remove", errorDetails);
            callback(errorDetails.userMessage); 
        },
        id
    );
}

// ============================================================================
// 6. GET GEOJSON (Non paginé)
// ============================================================================
void ObstacleService::getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // On récupère le type d'obstacle et sa géométrie au format GeoJSON
    std::string sql = "SELECT id, type, geom_type, ST_AsGeoJSON(geom) as geojson FROM obstacle ORDER BY id";

    client->execSqlAsync(sql, 
        [callback](const Result &r) {
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
                props["type"] = row["type"].as<std::string>();
                props["geom_type"] = row["geom_type"].as<std::string>();
                feature["properties"] = props;

                // Parsing de la géométrie GeoJSON brute
                std::string geoString = row["geojson"].as<std::string>();
                Json::Value geomObj;
                if(reader->parse(geoString.c_str(), geoString.c_str() + geoString.length(), &geomObj, &errs)){
                    feature["geometry"] = geomObj;
                }
                features.append(feature);
            }
            featureCollection["features"] = features;
            
            LOG_INFO << "Generated GeoJSON for " << features.size() << " obstacles";
            callback(featureCollection, "");
        },
        [callback](const DrogonDbException &e) { 
            Json::Value empty; 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("ObstacleService::getAllGeoJSON", errorDetails);
            callback(empty, errorDetails.userMessage); 
        }
    );
}

// ============================================================================
// 7. READ ALL PAGINATED (FIXED)
// ============================================================================
void ObstacleService::getAllPaginated(
    int page, 
    int pageSize,
    std::function<void(const std::vector<ObstacleModel>&, const PaginationMeta&, const std::string&)> callback
) {
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;

    int offset = (page - 1) * pageSize;
    
    // Capture du client pour réutilisation
    auto clientPtr = app().getDbClient();
    
    clientPtr->execSqlAsync(
        "SELECT COUNT(*) FROM obstacle",
        [callback, page, pageSize, offset, clientPtr](const Result& r) {
            int totalItems = r[0][0].as<int>();
            int totalPages = (totalItems + pageSize - 1) / pageSize;
            
            PaginationMeta meta;
            meta.currentPage = page;
            meta.pageSize = pageSize;
            meta.totalItems = totalItems;
            meta.totalPages = totalPages;
            meta.hasNext = page < totalPages;
            meta.hasPrev = page > 1;
            
            if (totalItems == 0) {
                callback({}, meta, "");
                return;
            }
            
            // Injection directe pour éviter le bug du driver
            std::string sql = "SELECT id, type, geom_type, ST_AsText(geom) as wkt "
                              "FROM obstacle ORDER BY id LIMIT " + std::to_string(pageSize) + 
                              " OFFSET " + std::to_string(offset);

            clientPtr->execSqlAsync(
                sql,
                [callback, meta](const Result& r) {
                    std::vector<ObstacleModel> list;
                    for (auto row : r) {
                        ObstacleModel o;
                        o.id = row["id"].as<int>();
                        o.type = row["type"].as<std::string>();
                        o.geom_type = row["geom_type"].as<std::string>();
                        o.wkt_geometry = row["wkt"].as<std::string>();
                        list.push_back(o);
                    }
                    callback(list, meta, "");
                },
                [callback, meta](const DrogonDbException& e) {
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("ObstacleService::getAllPaginated (data)", errorDetails);
                    callback({}, meta, errorDetails.userMessage);
                }
            );
        },
        [callback, page, pageSize](const DrogonDbException& e) {
            PaginationMeta emptyMeta = {page, pageSize, 0, 0, false, false};
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("ObstacleService::getAllPaginated (count)", errorDetails);
            callback({}, emptyMeta, errorDetails.userMessage);
        }
    );
}

// ============================================================================
// 8. GET ALL GEOJSON PAGINATED (FIXED)
// ============================================================================
void ObstacleService::getAllGeoJSONPaginated(
    int page,
    int pageSize,
    std::function<void(const Json::Value&, const PaginationMeta&, const std::string&)> callback
) {
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;

    int offset = (page - 1) * pageSize;
    
    // Capture du client pour réutilisation
    auto clientPtr = app().getDbClient();
    
    clientPtr->execSqlAsync(
        "SELECT COUNT(*) FROM obstacle",
        [callback, page, pageSize, offset, clientPtr](const Result& r) {
            int totalItems = r[0][0].as<int>();
            int totalPages = (totalItems + pageSize - 1) / pageSize;
            
            PaginationMeta meta;
            meta.currentPage = page;
            meta.pageSize = pageSize;
            meta.totalItems = totalItems;
            meta.totalPages = totalPages;
            meta.hasNext = page < totalPages;
            meta.hasPrev = page > 1;
            
            if (totalItems == 0) {
                Json::Value emptyGeoJSON;
                emptyGeoJSON["type"] = "FeatureCollection";
                emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                callback(emptyGeoJSON, meta, "");
                return;
            }
            
            // Injection directe pour éviter le bug du driver
            std::string sql = "SELECT id, type, geom_type, ST_AsGeoJSON(geom) as geojson "
                              "FROM obstacle ORDER BY id LIMIT " + std::to_string(pageSize) + 
                              " OFFSET " + std::to_string(offset);

            clientPtr->execSqlAsync(
                sql,
                [callback, meta](const Result& r) {
                    Json::Value featureCollection;
                    featureCollection["type"] = "FeatureCollection";
                    Json::Value features(Json::arrayValue);
                    
                    Json::CharReaderBuilder builder;
                    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                    std::string errs;
                    
                    for (auto row : r) {
                        Json::Value feature;
                        feature["type"] = "Feature";
                        
                        Json::Value props;
                        props["id"] = row["id"].as<int>();
                        props["type"] = row["type"].as<std::string>();
                        props["geom_type"] = row["geom_type"].as<std::string>();
                        feature["properties"] = props;
                        
                        std::string geoString = row["geojson"].as<std::string>();
                        Json::Value geomObj;
                        if(reader->parse(geoString.c_str(), geoString.c_str() + geoString.length(), &geomObj, &errs)){
                            feature["geometry"] = geomObj;
                        }
                        features.append(feature);
                    }
                    featureCollection["features"] = features;
                    callback(featureCollection, meta, "");
                },
                [callback, meta](const DrogonDbException& e) {
                    Json::Value empty;
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("ObstacleService::getAllGeoJSONPaginated (data)", errorDetails);
                    callback(empty, meta, errorDetails.userMessage);
                }
            );
        },
        [callback, page, pageSize](const DrogonDbException& e) {
            Json::Value empty;
            PaginationMeta emptyMeta = {page, pageSize, 0, 0, false, false};
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("ObstacleService::getAllGeoJSONPaginated (count)", errorDetails);
            callback(empty, emptyMeta, errorDetails.userMessage);
        }
    );
}

// ============================================================================
// GET OBSTACLES BY BOUNDING BOX
// ============================================================================
void ObstacleService::getByBoundingBox(double minLon, double minLat, double maxLon, double maxLat, const std::optional<std::string>& type, const std::function<void(const Json::Value&, const std::string&)>& callback) {
    auto dbClient = drogon::app().getDbClient();

    // Construct SQL query
    std::string sql = R"(SELECT jsonb_build_object(
        'type', 'FeatureCollection',
        'features', jsonb_agg(jsonb_build_object(
            'type', 'Feature',
            'geometry', ST_AsGeoJSON(t.geom)::jsonb,
            'properties', jsonb_build_object(
                'id', t.id,
                'type', t.type,
                'geom_type', t.geom_type
            )
        ))
    ) AS geojson
    FROM obstacle t
    WHERE ST_Intersects(t.geom, ST_MakeEnvelope($1, $2, $3, $4, 4326)))";

    if (type.has_value()) {
        sql += " AND t.type = $5";
        dbClient->execSqlAsync(
            sql,
            [callback](const drogon::orm::Result& result) {
                        if (!result.empty()) {
                            const auto& row = result[0];
                            std::string geoString = row["geojson"].as<std::string>();
                            Json::CharReaderBuilder builder;
                            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                            Json::Value parsed;
                            std::string errs;
                            if (reader->parse(geoString.c_str(), geoString.c_str() + geoString.length(), &parsed, &errs)) {
                                // Ensure 'features' is always an array (Postgres jsonb_agg returns null when no rows)
                                if (parsed.isObject() && parsed["features"].isNull()) {
                                    parsed["features"] = Json::Value(Json::arrayValue);
                                }
                                callback(parsed, "");
                            } else {
                                LOG_ERROR << "ObstacleService::getByBoundingBox - Failed to parse GeoJSON: " << errs;
                                Json::Value emptyGeoJSON;
                                emptyGeoJSON["type"] = "FeatureCollection";
                                emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                                callback(emptyGeoJSON, "");
                            }
                        } else {
                            Json::Value emptyGeoJSON;
                            emptyGeoJSON["type"] = "FeatureCollection";
                            emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                            callback(emptyGeoJSON, "");
                        }
            },
            [callback](const drogon::orm::DrogonDbException& e) {
                callback(Json::Value(), e.base().what());
            },
            minLon, minLat, maxLon, maxLat, type.value()
        );
    } else {
        dbClient->execSqlAsync(
            sql,
            [callback](const drogon::orm::Result& result) {
                        if (!result.empty()) {
                    const auto& row = result[0];
                    std::string geoString = row["geojson"].as<std::string>();
                    Json::CharReaderBuilder builder;
                    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                    Json::Value parsed;
                    std::string errs;
                    if (reader->parse(geoString.c_str(), geoString.c_str() + geoString.length(), &parsed, &errs)) {
                                // Ensure 'features' is an array (not null) for GeoJSON compliance
                                if (parsed.isObject() && parsed["features"].isNull()) {
                                    parsed["features"] = Json::Value(Json::arrayValue);
                                }
                                callback(parsed, "");
                    } else {
                        LOG_ERROR << "ObstacleService::getByBoundingBox - Failed to parse GeoJSON: " << errs;
                        Json::Value emptyGeoJSON;
                        emptyGeoJSON["type"] = "FeatureCollection";
                        emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                        callback(emptyGeoJSON, "");
                    }
                } else {
                    Json::Value emptyGeoJSON;
                    emptyGeoJSON["type"] = "FeatureCollection";
                    emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                    callback(emptyGeoJSON, "");
                }
            },
            [callback](const drogon::orm::DrogonDbException& e) {
                callback(Json::Value(), e.base().what());
            },
            minLon, minLat, maxLon, maxLat
        );
    }
}
