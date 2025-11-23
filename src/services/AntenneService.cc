#include "AntenneService.h"
#include "../utils/ErrorHandler.h"

#include <json/json.h>
#include <optional>

using namespace drogon;
using namespace drogon::orm;

// ============================================================================
// 1. CREATE avec gestion d'erreurs améliorée
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
// 2. READ ALL (sans pagination - compatibilité rétroactive)
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
// 2bis. READ ALL PAGINATED (nouvelle version avec pagination)
// ============================================================================
void AntenneService::getAllPaginated(
    int page, 
    int pageSize,
    std::function<void(const std::vector<Antenna>&, const PaginationMeta&, const std::string&)> callback) 
{
    auto client = app().getDbClient();
    
    // Validation des paramètres
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;
    
    int offset = (page - 1) * pageSize;
    
    // 1. Compter le total
    std::string countSql = "SELECT COUNT(*) as total FROM antenna";
    
    client->execSqlAsync(
        countSql,
        [callback, page, pageSize, offset, client](const Result &countResult) {
            int totalItems = countResult[0]["total"].as<int>();
            int totalPages = (totalItems + pageSize - 1) / pageSize;
            
            PaginationMeta meta;
            meta.currentPage = page;
            meta.pageSize = pageSize;
            meta.totalItems = totalItems;
            meta.totalPages = totalPages;
            meta.hasNext = page < totalPages;
            meta.hasPrev = page > 1;
            
            // 2. Récupérer les données paginées
            std::string sql =
                "SELECT id, coverage_radius, status, technology, "
                "installation_date::text AS installation_date, "
                "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat "
                "FROM antenna "
                "ORDER BY id "
                "LIMIT $1 OFFSET $2";
            
            client->execSqlAsync(
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
                    
                    LOG_INFO << "Page " << meta.currentPage << ": " << list.size() 
                             << " items (total: " << meta.totalItems << ")";
                    
                    callback(list, meta, "");
                },
                [callback, meta](const DrogonDbException &e) {
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("AntenneService::getAllPaginated", errorDetails);
                    callback({}, meta, errorDetails.userMessage);
                },
                pageSize,
                offset
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
// 3. READ ONE avec gestion d'erreurs améliorée
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
                LOG_WARN << "Antenna with ID " << id << " not found";
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
            
            LOG_INFO << "Retrieved antenna ID " << id << " successfully";
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
// 4. UPDATE avec gestion d'erreurs améliorée
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
                LOG_WARN << "Update failed: Antenna ID " << a.id << " not found";
                callback("Not Found");
            } else {
                LOG_INFO << "Antenna ID " << a.id << " updated successfully";
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
// 5. DELETE avec gestion d'erreurs améliorée
// ============================================================================
void AntenneService::remove(int id, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    
    client->execSqlAsync(
        "DELETE FROM antenna WHERE id = $1",
        [callback, id](const Result &r) { 
            if (r.affectedRows() == 0) {
                LOG_WARN << "Delete failed: Antenna ID " << id << " not found";
                callback("Not Found");
            } else {
                LOG_INFO << "Antenna ID " << id << " deleted successfully";
                callback(""); 
            }
        },
        [callback](const DrogonDbException &e) { 
            auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
            ErrorHandler::logError("AntenneService::remove", errorDetails);
            
            if (errorDetails.type == ErrorHandler::ErrorType::FOREIGN_KEY_VIOLATION) {
                callback("Cannot delete antenna: it is referenced by other entities (zones, reports, etc.)");
            } else {
                callback(errorDetails.userMessage);
            }
        }, 
        id
    );
}

// ============================================================================
// 6. SEARCH IN RADIUS (sans pagination)
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
        [callback, lat, lon, radiusMeters](const Result &r) {
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
            
            LOG_INFO << "Found " << list.size() << " antennas within " << radiusMeters 
                     << "m of (" << lat << ", " << lon << ")";
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
    auto client = app().getDbClient();
    
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;
    
    int offset = (page - 1) * pageSize;
    
    std::string countSql =
        "SELECT COUNT(*) as total FROM antenna "
        "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3)";
    
    client->execSqlAsync(
        countSql,
        [callback, lat, lon, radiusMeters, page, pageSize, offset, client](const Result &countResult) {
            int totalItems = countResult[0]["total"].as<int>();
            int totalPages = (totalItems + pageSize - 1) / pageSize;
            
            PaginationMeta meta;
            meta.currentPage = page;
            meta.pageSize = pageSize;
            meta.totalItems = totalItems;
            meta.totalPages = totalPages;
            meta.hasNext = page < totalPages;
            meta.hasPrev = page > 1;
            
            std::string sql =
                "SELECT id, coverage_radius, status, technology, "
                "installation_date::text AS installation_date, "
                "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat, "
                "ST_Distance(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography) AS distance "
                "FROM antenna "
                "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3) "
                "ORDER BY distance "
                "LIMIT $4 OFFSET $5";
            
            client->execSqlAsync(
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
                    
                    LOG_INFO << "Radius search page " << meta.currentPage << ": " 
                             << list.size() << " items (total: " << meta.totalItems << ")";
                    
                    callback(list, meta, "");
                },
                [callback, meta](const DrogonDbException &e) {
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("AntenneService::getInRadiusPaginated", errorDetails);
                    callback({}, meta, errorDetails.userMessage);
                },
                lon, lat, radiusMeters, pageSize, offset
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
// 7. GET GEOJSON (sans pagination)
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
                if (reader->parse(geojsonString.c_str(), geojsonString.c_str() + geojsonString.length(),
                                  &geometryObject, &errs)) {
                    feature["geometry"] = geometryObject;
                } else {
                    LOG_ERROR << "Failed to parse GeoJSON for antenna ID " << row["id"].as<int>() 
                              << ": " << errs;
                }

                features.append(feature);
            }

            featureCollection["features"] = features;
            
            LOG_INFO << "Generated GeoJSON with " << features.size() << " features";
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
// 7bis. GET GEOJSON PAGINATED (optimisé pour Leaflet)
// ============================================================================
void AntenneService::getAllGeoJSONPaginated(
    int page, int pageSize,
    std::function<void(const Json::Value&, const PaginationMeta&, const std::string&)> callback)
{
    auto client = app().getDbClient();
    
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;
    
    int offset = (page - 1) * pageSize;
    
    std::string countSql = "SELECT COUNT(*) as total FROM antenna";
    
    client->execSqlAsync(
        countSql,
        [callback, page, pageSize, offset, client](const Result &countResult) {
            int totalItems = countResult[0]["total"].as<int>();
            int totalPages = (totalItems + pageSize - 1) / pageSize;
            
            PaginationMeta meta;
            meta.currentPage = page;
            meta.pageSize = pageSize;
            meta.totalItems = totalItems;
            meta.totalPages = totalPages;
            meta.hasNext = page < totalPages;
            meta.hasPrev = page > 1;
            
            std::string sql =
                "SELECT id, coverage_radius, status, technology, "
                "installation_date::text AS installation_date, "
                "operator_id, ST_AsGeoJSON(geom) AS geojson "
                "FROM antenna "
                "ORDER BY id "
                "LIMIT $1 OFFSET $2";
            
            client->execSqlAsync(
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
                        if (reader->parse(geojsonString.c_str(), 
                                         geojsonString.c_str() + geojsonString.length(),
                                         &geometryObject, &errs)) {
                            feature["geometry"] = geometryObject;
                        } else {
                            LOG_ERROR << "Failed to parse GeoJSON for antenna ID " 
                                     << row["id"].as<int>() << ": " << errs;
                        }

                        features.append(feature);
                    }

                    featureCollection["features"] = features;
                    
                    LOG_INFO << "GeoJSON page " << meta.currentPage << ": " 
                             << features.size() << " features (total: " << meta.totalItems << ")";
                    
                    callback(featureCollection, meta, "");
                },
                [callback, meta](const DrogonDbException &e) {
                    Json::Value empty;
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("AntenneService::getAllGeoJSONPaginated", errorDetails);
                    callback(empty, meta, errorDetails.userMessage);
                },
                pageSize,
                offset
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
// 8. GET GEOJSON IN RADIUS (pour carte dynamique Leaflet)
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
            
            // Métadonnées de la recherche
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
                if (reader->parse(geojsonString.c_str(), 
                                 geojsonString.c_str() + geojsonString.length(),
                                 &geometryObject, &errs)) {
                    feature["geometry"] = geometryObject;
                }

                features.append(feature);
            }

            featureCollection["features"] = features;
            
            LOG_INFO << "GeoJSON radius search: " << features.size() 
                     << " features within " << radiusMeters << "m";
            
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
// 9. GET GEOJSON IN BBOX (pour viewport Leaflet)
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
            
            // Métadonnées de la bbox
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
                if (reader->parse(geojsonString.c_str(), 
                                 geojsonString.c_str() + geojsonString.length(),
                                 &geometryObject, &errs)) {
                    feature["geometry"] = geometryObject;
                }

                features.append(feature);
            }

            featureCollection["features"] = features;
            
            LOG_INFO << "GeoJSON bbox query: " << features.size() << " features";
            
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
// 10. GET GEOJSON IN RADIUS PAGINATED (combinaison rayon + pagination)
// ============================================================================
void AntenneService::getGeoJSONInRadiusPaginated(
    double lat, double lon, double radiusMeters,
    int page, int pageSize,
    std::function<void(const Json::Value&, const PaginationMeta&, const std::string&)> callback)
{
    auto client = app().getDbClient();
    
    if (page < 1) page = 1;
    if (pageSize < 1) pageSize = 20;
    if (pageSize > 100) pageSize = 100;
    
    int offset = (page - 1) * pageSize;
    
    std::string countSql =
        "SELECT COUNT(*) as total FROM antenna "
        "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3)";
    
    client->execSqlAsync(
        countSql,
        [callback, lat, lon, radiusMeters, page, pageSize, offset, client](const Result &countResult) {
            int totalItems = countResult[0]["total"].as<int>();
            int totalPages = (totalItems + pageSize - 1) / pageSize;
            
            PaginationMeta meta;
            meta.currentPage = page;
            meta.pageSize = pageSize;
            meta.totalItems = totalItems;
            meta.totalPages = totalPages;
            meta.hasNext = page < totalPages;
            meta.hasPrev = page > 1;
            
            std::string sql =
                "SELECT id, coverage_radius, status, technology, "
                "installation_date::text AS installation_date, "
                "operator_id, ST_AsGeoJSON(geom) AS geojson, "
                "ST_Distance(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography) AS distance "
                "FROM antenna "
                "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3) "
                "ORDER BY distance "
                "LIMIT $4 OFFSET $5";
            
            client->execSqlAsync(
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
                        if (reader->parse(geojsonString.c_str(), 
                                         geojsonString.c_str() + geojsonString.length(),
                                         &geometryObject, &errs)) {
                            feature["geometry"] = geometryObject;
                        }

                        features.append(feature);
                    }

                    featureCollection["features"] = features;
                    
                    LOG_INFO << "GeoJSON radius search page " << meta.currentPage << ": " 
                             << features.size() << " features (total: " << meta.totalItems << ")";
                    
                    callback(featureCollection, meta, "");
                },
                [callback, meta](const DrogonDbException &e) {
                    Json::Value empty;
                    auto errorDetails = ErrorHandler::analyzePostgresError(e.base().what());
                    ErrorHandler::logError("AntenneService::getGeoJSONInRadiusPaginated", errorDetails);
                    callback(empty, meta, errorDetails.userMessage);
                },
                lon, lat, radiusMeters, pageSize, offset
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