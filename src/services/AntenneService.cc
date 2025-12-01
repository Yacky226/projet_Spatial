#include "AntenneService.h"
#include "../utils/ErrorHandler.h"

#include <json/json.h>
#include <optional>
#include <string>

using namespace drogon;
using namespace drogon::orm;

// ============================================================================
// 1. CREATE
// ============================================================================
void AntenneService::create(const Antenna &a, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "INSERT INTO antenna (coverage_radius, status, technology, installation_date, operator_id, geom) "
        "VALUES ($1, $2::antenna_status, $3::technology_type, $4, $5, ST_SetSRID(ST_MakePoint($6, $7), 4326)) "
        "RETURNING id";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) { 
            int newId = r[0]["id"].as<int>();
            LOG_INFO << "Antenna created successfully with ID: " << newId;
            callback(""); 
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::create", errorDetails);
            callback(errorDetails.userMessage);
        }, 
        a.coverage_radius,
        a.status, 
        a.technology,
        (a.installation_date.empty() ? std::nullopt : std::make_optional(a.installation_date)),
        a.operator_id, 
        a.longitude, 
        a.latitude
    );
}

// ============================================================================
// 2. READ ALL (Non paginé)
// ============================================================================
void AntenneService::getAll(std::function<void(const std::vector<Antenna> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "SELECT id, coverage_radius, status, technology, installation_date::text AS installation_date, "
        "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat FROM antenna ORDER BY id";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<Antenna> list;
            for (auto row : r) {
                Antenna a;
                a.id = row["id"].as<int>();
                a.coverage_radius = row["coverage_radius"].as<double>();
                a.status = row["status"].as<std::string>();
                a.technology = row["technology"].as<std::string>();
                a.installation_date =
                    row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                a.operator_id = row["operator_id"].as<int>();
                a.longitude = row["lon"].as<double>();
                a.latitude = row["lat"].as<double>();
                list.push_back(a);
            }
            LOG_INFO << "Retrieved " << list.size() << " antennas successfully";
            callback(list, "");
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getAll", errorDetails);
            callback({}, errorDetails.userMessage); 
        }
    );
}

// ============================================================================
// 2bis. READ ALL PAGINATED
// ============================================================================
void AntenneService::getAllPaginated(
    int page, 
    int pageSize,
    std::function<void(const std::vector<Antenna>&, const PaginationMeta&, const std::string&)> callback) 
{
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;
    
    int offset = (page - 1) * pageSize;
    
    // On capture le client pour le réutiliser
    auto clientPtr = app().getDbClient();
    std::string countSql = "SELECT COUNT(*) as total FROM antenna";
    
    clientPtr->execSqlAsync(
        countSql,
        [callback, page, pageSize, offset, clientPtr](const Result &countResult) {
            int totalItems = countResult[0]["total"].as<int>();
            int totalPages = totalItems > 0 ? (totalItems + pageSize - 1) / pageSize : 0;
            
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
            
            // FIX: Injection directe des entiers dans la chaîne pour éviter le bug de binding paramètre
            std::string sql =
                "SELECT id, coverage_radius, status, technology, "
                "installation_date::text AS installation_date, "
                "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat "
                "FROM antenna "
                "ORDER BY id "
                "LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset);
            
            clientPtr->execSqlAsync(
                sql,
                [callback, meta](const Result &r) {
                    std::vector<Antenna> list;
                    for (auto row : r) {
                        Antenna a;
                        a.id = row["id"].as<int>();
                        a.coverage_radius = row["coverage_radius"].as<double>();
                        a.status = row["status"].as<std::string>();
                        a.technology = row["technology"].as<std::string>();
                        a.installation_date =
                            row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                        a.operator_id = row["operator_id"].as<int>();
                        a.longitude = row["lon"].as<double>();
                        a.latitude = row["lat"].as<double>();
                        list.push_back(a);
                    }
                    callback(list, meta, "");
                },
                [callback, meta](const DrogonDbException &e) {
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("AntenneService::getAllPaginated (data)", errorDetails);
                    callback({}, meta, errorDetails.userMessage);
                }
            );
        },
        [callback, page, pageSize](const DrogonDbException &e) {
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getAllPaginated (count)", errorDetails);
            PaginationMeta emptyMeta = {page, pageSize, 0, 0, false, false};
            callback({}, emptyMeta, errorDetails.userMessage);
        }
    );
}

// ============================================================================
// 3. READ ONE
// ============================================================================
void AntenneService::getById(int id, std::function<void(const Antenna &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "SELECT id, coverage_radius, status, technology, installation_date::text AS installation_date, "
        "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat FROM antenna WHERE id = $1";

    client->execSqlAsync(
        sql,
        [callback, id](const Result &r) {
            if (r.empty()) {
                callback({}, "Not Found");
                return;
            }
            Antenna a;
            auto row = r[0];
            a.id = row["id"].as<int>();
            a.coverage_radius = row["coverage_radius"].as<double>();
            a.status = row["status"].as<std::string>();
            a.technology = row["technology"].as<std::string>();
            a.installation_date =
                row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
            a.operator_id = row["operator_id"].as<int>();
            a.longitude = row["lon"].as<double>();
            a.latitude = row["lat"].as<double>();
            callback(a, "");
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getById", errorDetails);
            callback({}, errorDetails.userMessage); 
        }, 
        id
    );
}

// ============================================================================
// 4. UPDATE
// ============================================================================
void AntenneService::update(const Antenna &a, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "UPDATE antenna SET coverage_radius=$1, status=$2::antenna_status, technology=$3::technology_type, "
        "installation_date=$4, operator_id=$5, geom=ST_SetSRID(ST_MakePoint($6, $7), 4326) WHERE id=$8";

    client->execSqlAsync(
        sql,
        [callback, a](const Result &r) { 
            if (r.affectedRows() == 0) {
                callback("Not Found");
            } else {
                callback(""); 
            }
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::update", errorDetails);
            callback(errorDetails.userMessage); 
        }, 
        a.coverage_radius,
        a.status, 
        a.technology,
        (a.installation_date.empty() ? std::nullopt : std::make_optional(a.installation_date)),
        a.operator_id, 
        a.longitude, 
        a.latitude, 
        a.id
    );
}

// ============================================================================
// 5. DELETE
// ============================================================================
void AntenneService::remove(int id, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync(
        "DELETE FROM antenna WHERE id = $1",
        [callback, id](const Result &r) { 
            if (r.affectedRows() == 0) {
                callback("Not Found");
            } else {
                callback(""); 
            }
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::remove", errorDetails);
            if (errorDetails.type == ErrorHandler::ErrorType::FOREIGN_KEY_VIOLATION) {
                callback("Cannot delete antenna: it is referenced by other entities");
            } else {
                callback(errorDetails.userMessage);
            }
        }, 
        id
    );
}

// ============================================================================
// 6. SEARCH IN RADIUS (Non paginé)
// ============================================================================
void AntenneService::getInRadius(double lat, double lon, double radiusMeters,
                                 std::function<void(const std::vector<Antenna> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "SELECT id, coverage_radius, status, technology, installation_date::text AS installation_date, "
        "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat "
        "FROM antenna "
        "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3) "
        "ORDER BY ST_Distance(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography)";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<Antenna> list;
            for (auto row : r) {
                Antenna a;
                a.id = row["id"].as<int>();
                a.coverage_radius = row["coverage_radius"].as<double>();
                a.status = row["status"].as<std::string>();
                a.technology = row["technology"].as<std::string>();
                a.installation_date =
                    row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                a.operator_id = row["operator_id"].as<int>();
                a.longitude = row["lon"].as<double>();
                a.latitude = row["lat"].as<double>();
                list.push_back(a);
            }
            callback(list, "");
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getInRadius", errorDetails);
            callback({}, errorDetails.userMessage); 
        }, 
        lon, lat, radiusMeters
    );
}

// ============================================================================
// 6bis. SEARCH IN RADIUS PAGINATED
// ============================================================================
void AntenneService::getInRadiusPaginated(
    double lat, double lon, double radiusMeters,
    int page, int pageSize,
    std::function<void(const std::vector<Antenna>&, const PaginationMeta&, const std::string&)> callback)
{
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;
    
    int offset = (page - 1) * pageSize;
    auto clientPtr = app().getDbClient();
    
    std::string countSql =
        "SELECT COUNT(*) as total FROM antenna "
        "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3)";
    
    clientPtr->execSqlAsync(
        countSql,
        [callback, lat, lon, radiusMeters, page, pageSize, offset, clientPtr](const Result &countResult) {
            int totalItems = countResult[0]["total"].as<int>();
            int totalPages = totalItems > 0 ? (totalItems + pageSize - 1) / pageSize : 0;
            
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
            
            // FIX: Injection directe LIMIT/OFFSET
            std::string sql =
                "SELECT id, coverage_radius, status, technology, "
                "installation_date::text AS installation_date, "
                "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat, "
                "ST_Distance(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography) AS distance "
                "FROM antenna "
                "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3) "
                "ORDER BY distance "
                "LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset);
            
            clientPtr->execSqlAsync(
                sql,
                [callback, meta](const Result &r) {
                    std::vector<Antenna> list;
                    for (auto row : r) {
                        Antenna a;
                        a.id = row["id"].as<int>();
                        a.coverage_radius = row["coverage_radius"].as<double>();
                        a.status = row["status"].as<std::string>();
                        a.technology = row["technology"].as<std::string>();
                        a.installation_date =
                            row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                        a.operator_id = row["operator_id"].as<int>();
                        a.longitude = row["lon"].as<double>();
                        a.latitude = row["lat"].as<double>();
                        list.push_back(a);
                    }
                    callback(list, meta, "");
                },
                [callback, meta](const DrogonDbException &e) {
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("AntenneService::getInRadiusPaginated (data)", errorDetails);
                    callback({}, meta, errorDetails.userMessage);
                },
                lon, lat, radiusMeters
            );
        },
        [callback, page, pageSize](const DrogonDbException &e) {
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getInRadiusPaginated (count)", errorDetails);
            PaginationMeta emptyMeta = {page, pageSize, 0, 0, false, false};
            callback({}, emptyMeta, errorDetails.userMessage);
        },
        lon, lat, radiusMeters
    );
}

// ============================================================================
// 7. GET GEOJSON (Non paginé)
// ============================================================================
void AntenneService::getAllGeoJSON(std::function<void(const Json::Value &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "SELECT id, coverage_radius, status, technology, installation_date::text AS installation_date, "
        "operator_id, ST_AsGeoJSON(geom) AS geojson FROM antenna ORDER BY id";

    client->execSqlAsync(
        sql,
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
                Json::Value props;
                props["id"] = row["id"].as<int>();
                props["technology"] = row["technology"].as<std::string>();
                props["status"] = row["status"].as<std::string>();
                props["coverage_radius"] = row["coverage_radius"].as<double>();
                props["operator_id"] = row["operator_id"].as<int>();
                props["installation_date"] =
                    row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                feature["properties"] = props;

                std::string geojsonString = row["geojson"].as<std::string>();
                Json::Value geometryObject;
                if (reader->parse(geojsonString.c_str(), geojsonString.c_str() + geojsonString.length(), &geometryObject, &errs)) {
                    feature["geometry"] = geometryObject;
                }
                features.append(feature);
            }
            featureCollection["features"] = features;
            callback(featureCollection, "");
        },
        [callback](const DrogonDbException &e) {
            Json::Value empty;
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getAllGeoJSON", errorDetails);
            callback(empty, errorDetails.userMessage);
        }
    );
}

// ============================================================================
// 7bis. GET GEOJSON PAGINATED
// ============================================================================
void AntenneService::getAllGeoJSONPaginated(
    int page, int pageSize,
    std::function<void(const Json::Value&, const PaginationMeta&, const std::string&)> callback)
{
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;
    
    int offset = (page - 1) * pageSize;
    auto clientPtr = app().getDbClient();
    std::string countSql = "SELECT COUNT(*) as total FROM antenna";
    
    clientPtr->execSqlAsync(
        countSql,
        [callback, page, pageSize, offset, clientPtr](const Result &countResult) {
            int totalItems = countResult[0]["total"].as<int>();
            int totalPages = totalItems > 0 ? (totalItems + pageSize - 1) / pageSize : 0;
            
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
            
            // FIX: Injection directe LIMIT/OFFSET
            std::string sql =
                "SELECT id, coverage_radius, status, technology, "
                "installation_date::text AS installation_date, "
                "operator_id, ST_AsGeoJSON(geom) AS geojson "
                "FROM antenna "
                "ORDER BY id "
                "LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset);
            
            clientPtr->execSqlAsync(
                sql,
                [callback, meta](const Result &r) {
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
                        props["technology"] = row["technology"].as<std::string>();
                        props["status"] = row["status"].as<std::string>();
                        props["coverage_radius"] = row["coverage_radius"].as<double>();
                        props["operator_id"] = row["operator_id"].as<int>();
                        props["installation_date"] =
                            row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                        feature["properties"] = props;

                        std::string geojsonString = row["geojson"].as<std::string>();
                        Json::Value geometryObject;
                        if (reader->parse(geojsonString.c_str(), geojsonString.c_str() + geojsonString.length(), &geometryObject, &errs)) {
                            feature["geometry"] = geometryObject;
                        }
                        features.append(feature);
                    }
                    featureCollection["features"] = features;
                    callback(featureCollection, meta, "");
                },
                [callback, meta](const DrogonDbException &e) {
                    Json::Value empty;
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("AntenneService::getAllGeoJSONPaginated (data)", errorDetails);
                    callback(empty, meta, errorDetails.userMessage);
                }
            );
        },
        [callback, page, pageSize](const DrogonDbException &e) {
            Json::Value empty;
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getAllGeoJSONPaginated (count)", errorDetails);
            PaginationMeta emptyMeta = {page, pageSize, 0, 0, false, false};
            callback(empty, emptyMeta, errorDetails.userMessage);
        }
    );
}

// ============================================================================
// 8. GET GEOJSON IN RADIUS
// ============================================================================
void AntenneService::getGeoJSONInRadius(
    double lat, double lon, double radiusMeters,
    std::function<void(const Json::Value&, const std::string&)> callback)
{
    auto client = app().getDbClient();
    std::string sql =
        "SELECT id, coverage_radius, status, technology, "
        "installation_date::text AS installation_date, "
        "operator_id, ST_AsGeoJSON(geom) AS geojson, "
        "ST_Distance(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography) AS distance "
        "FROM antenna "
        "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3) "
        "ORDER BY distance";
    
    client->execSqlAsync(
        sql,
        [callback, lat, lon, radiusMeters](const Result &r) {
            Json::Value featureCollection;
            featureCollection["type"] = "FeatureCollection";
            
            Json::Value searchMeta;
            searchMeta["center"] = Json::Value(Json::arrayValue);
            searchMeta["center"].append(lon);
            searchMeta["center"].append(lat);
            searchMeta["radius"] = radiusMeters;
            searchMeta["count"] = static_cast<int>(r.size());
            featureCollection["metadata"] = searchMeta;
            
            Json::Value features(Json::arrayValue);
            Json::CharReaderBuilder builder;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            std::string errs;

            for (auto row : r) {
                Json::Value feature;
                feature["type"] = "Feature";
                Json::Value props;
                props["id"] = row["id"].as<int>();
                props["technology"] = row["technology"].as<std::string>();
                props["status"] = row["status"].as<std::string>();
                props["coverage_radius"] = row["coverage_radius"].as<double>();
                props["operator_id"] = row["operator_id"].as<int>();
                props["installation_date"] =
                    row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                props["distance"] = row["distance"].as<double>();
                feature["properties"] = props;

                std::string geojsonString = row["geojson"].as<std::string>();
                Json::Value geometryObject;
                if (reader->parse(geojsonString.c_str(), geojsonString.c_str() + geojsonString.length(), &geometryObject, &errs)) {
                    feature["geometry"] = geometryObject;
                }
                features.append(feature);
            }
            featureCollection["features"] = features;
            callback(featureCollection, "");
        },
        [callback](const DrogonDbException &e) {
            Json::Value empty;
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getGeoJSONInRadius", errorDetails);
            callback(empty, errorDetails.userMessage);
        },
        lon, lat, radiusMeters
    );
}

// ============================================================================
// 9. GET GEOJSON IN BBOX
// ============================================================================
void AntenneService::getGeoJSONInBBox(
    double minLat, double minLon, double maxLat, double maxLon,
    std::function<void(const Json::Value&, const std::string&)> callback)
{
    auto client = app().getDbClient();
    
    std::string sql =
        "SELECT id, coverage_radius, status, technology, "
        "installation_date::text AS installation_date, "
        "operator_id, ST_AsGeoJSON(geom) AS geojson "
        "FROM antenna "
        "WHERE geom && ST_MakeEnvelope($1, $2, $3, $4, 4326) "
        "ORDER BY id";
    
    client->execSqlAsync(
        sql,
        [callback, minLat, minLon, maxLat, maxLon](const Result &r) {
            Json::Value featureCollection;
            featureCollection["type"] = "FeatureCollection";
            
            Json::Value bboxMeta;
            bboxMeta["bbox"] = Json::Value(Json::arrayValue);
            bboxMeta["bbox"].append(minLon);
            bboxMeta["bbox"].append(minLat);
            bboxMeta["bbox"].append(maxLon);
            bboxMeta["bbox"].append(maxLat);
            bboxMeta["count"] = static_cast<int>(r.size());
            featureCollection["metadata"] = bboxMeta;
            
            Json::Value features(Json::arrayValue);
            Json::CharReaderBuilder builder;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            std::string errs;

            for (auto row : r) {
                Json::Value feature;
                feature["type"] = "Feature";
                Json::Value props;
                props["id"] = row["id"].as<int>();
                props["technology"] = row["technology"].as<std::string>();
                props["status"] = row["status"].as<std::string>();
                props["coverage_radius"] = row["coverage_radius"].as<double>();
                props["operator_id"] = row["operator_id"].as<int>();
                props["installation_date"] =
                    row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                feature["properties"] = props;

                std::string geojsonString = row["geojson"].as<std::string>();
                Json::Value geometryObject;
                if (reader->parse(geojsonString.c_str(), geojsonString.c_str() + geojsonString.length(), &geometryObject, &errs)) {
                    feature["geometry"] = geometryObject;
                }
                features.append(feature);
            }
            featureCollection["features"] = features;
            callback(featureCollection, "");
        },
        [callback](const DrogonDbException &e) {
            Json::Value empty;
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getGeoJSONInBBox", errorDetails);
            callback(empty, errorDetails.userMessage);
        },
        minLon, minLat, maxLon, maxLat
    );
}

// ============================================================================
// 10. GET GEOJSON IN RADIUS PAGINATED
// ============================================================================
void AntenneService::getGeoJSONInRadiusPaginated(
    double lat, double lon, double radiusMeters,
    int page, int pageSize,
    std::function<void(const Json::Value&, const PaginationMeta&, const std::string&)> callback)
{
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;
    
    int offset = (page - 1) * pageSize;
    auto clientPtr = app().getDbClient();
    
    std::string countSql =
        "SELECT COUNT(*) as total FROM antenna "
        "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3)";
    
    clientPtr->execSqlAsync(
        countSql,
        [callback, lat, lon, radiusMeters, page, pageSize, offset, clientPtr](const Result &countResult) {
            int totalItems = countResult[0]["total"].as<int>();
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
            
            // FIX: Injection directe LIMIT/OFFSET
            std::string sql =
                "SELECT id, coverage_radius, status, technology, "
                "installation_date::text AS installation_date, "
                "operator_id, ST_AsGeoJSON(geom) AS geojson, "
                "ST_Distance(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography) AS distance "
                "FROM antenna "
                "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3) "
                "ORDER BY distance "
                "LIMIT " + std::to_string(pageSize) + " OFFSET " + std::to_string(offset);
            
            clientPtr->execSqlAsync(
                sql,
                [callback, meta, lat, lon, radiusMeters](const Result &r) {
                    Json::Value featureCollection;
                    featureCollection["type"] = "FeatureCollection";
                    
                    Json::Value searchMeta;
                    searchMeta["center"] = Json::Value(Json::arrayValue);
                    searchMeta["center"].append(lon);
                    searchMeta["center"].append(lat);
                    searchMeta["radius"] = radiusMeters;
                    featureCollection["metadata"] = searchMeta;
                    
                    Json::Value features(Json::arrayValue);
                    Json::CharReaderBuilder builder;
                    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                    std::string errs;

                    for (auto row : r) {
                        Json::Value feature;
                        feature["type"] = "Feature";
                        Json::Value props;
                        props["id"] = row["id"].as<int>();
                        props["technology"] = row["technology"].as<std::string>();
                        props["status"] = row["status"].as<std::string>();
                        props["coverage_radius"] = row["coverage_radius"].as<double>();
                        props["operator_id"] = row["operator_id"].as<int>();
                        props["installation_date"] =
                            row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                        props["distance"] = row["distance"].as<double>();
                        feature["properties"] = props;

                        std::string geojsonString = row["geojson"].as<std::string>();
                        Json::Value geometryObject;
                        if (reader->parse(geojsonString.c_str(), geojsonString.c_str() + geojsonString.length(), &geometryObject, &errs)) {
                            feature["geometry"] = geometryObject;
                        }
                        features.append(feature);
                    }
                    featureCollection["features"] = features;
                    callback(featureCollection, meta, "");
                },
                [callback, meta](const DrogonDbException &e) {
                    Json::Value empty;
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("AntenneService::getGeoJSONInRadiusPaginated (data)", errorDetails);
                    callback(empty, meta, errorDetails.userMessage);
                },
                lon, lat, radiusMeters
            );
        },
        [callback, page, pageSize](const DrogonDbException &e) {
            Json::Value empty;
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getGeoJSONInRadiusPaginated (count)", errorDetails);
            PaginationMeta emptyMeta = {page, pageSize, 0, 0, false, false};
            callback(empty, emptyMeta, errorDetails.userMessage);
        },
        lon, lat, radiusMeters
    );
}

// ============================================================================
// 11. GET COVERAGE BY OPERATOR (Calcul PostGIS)
// ============================================================================
void AntenneService::getCoverageByOperator(
    int operator_id, double minLat, double minLon, double maxLat, double maxLon,
    std::function<void(const Json::Value&, const std::string&)> callback)
{
    auto client = app().getDbClient();
    
    // Celui-ci effectue les opérations suivantes :
    // 1. Filtre par opérateur et zone (BBox)
    // 2. Transforme chaque point en cercle de couverture (ST_Buffer sur geography pour la précision en mètres)
    // 3. Fusionne tous les cercles en une seule forme géométrique (ST_Union)
    // 4. Renvoie le résultat en GeoJSON
    std::string sql = 
        "SELECT ST_AsGeoJSON(ST_Union(ST_Buffer(geom::geography, coverage_radius)::geometry)) as geojson "
        "FROM antenna "
        "WHERE operator_id = $1 "
        "AND geom && ST_MakeEnvelope($2, $3, $4, $5, 4326)";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            Json::Value featureCollection;
            featureCollection["type"] = "FeatureCollection";
            Json::Value features(Json::arrayValue);
            
            // Si on a des résultats (et que l'union n'est pas nulle)
            if (r.size() > 0 && !r[0]["geojson"].isNull()) {
                Json::Value feature;
                feature["type"] = "Feature";
                feature["properties"] = Json::Value(Json::objectValue); // Pas de propriétés spécifiques ici
                
                // Parsing du GeoJSON
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
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::getCoverageByOperator", errorDetails);
            callback(empty, errorDetails.userMessage);
        },
        operator_id, minLon, minLat, maxLon, maxLat // Attention à l'ordre pour ST_MakeEnvelope (X, Y, X, Y)
    );
}

// ============================================================================
// NOUVEAU : VORONOI DIAGRAM
// ============================================================================
void AntenneService::getVoronoiDiagram(
    std::function<void(const Json::Value&, const std::string&)> callback) {
    
    auto client = app().getDbClient();
    
    std::string sql = R"(
        WITH active_antennas AS (
            SELECT id, geom 
            FROM antenna 
            WHERE status = 'active'
        ),
        voronoi_cells AS (
            SELECT 
                (ST_Dump(ST_VoronoiPolygons(ST_Collect(geom)))).geom as cell,
                id
            FROM active_antennas
        ),
        attributed_cells AS (
            SELECT 
                a.id as antenna_id,
                a.geom as antenna_location,
                v.cell as voronoi_polygon
            FROM active_antennas a
            JOIN voronoi_cells v ON ST_Contains(v.cell, a.geom)
        )
        SELECT json_build_object(
            'type', 'FeatureCollection',
            'features', json_agg(
                json_build_object(
                    'type', 'Feature',
                    'id', antenna_id,
                    'geometry', ST_AsGeoJSON(voronoi_polygon)::json,
                    'properties', json_build_object(
                        'antenna_id', antenna_id,
                        'area_km2', ST_Area(voronoi_polygon::geography) / 1000000.0
                    )
                )
            )
        ) as geojson
        FROM attributed_cells
    )";
    
    client->execSqlAsync(sql,
        [callback](const Result& r) {
            if (!r.empty()) {
                std::string geojsonStr = r[0]["geojson"].as<std::string>();
                Json::Value geojson;
                Json::Reader reader;
                reader.parse(geojsonStr, geojson);
                callback(geojson, "");
            } else {
                callback(Json::Value(), "No antennas found");
            }
        },
        [callback](const DrogonDbException& e) {
            callback(Json::Value(), e.base().what());
        }
    );
}

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