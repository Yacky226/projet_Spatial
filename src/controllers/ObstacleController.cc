#include "ObstacleController.h"
#include "../utils/Validator.h"
#include "../utils/ErrorHandler.h"
#include "../utils/PaginationHelper.h"
#include <drogon/HttpResponse.h>
#include <sstream>

using namespace drogon;

// ============================================================================
// 1. CREATE avec validation
// ============================================================================
void ObstacleController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    
    if (!json) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid JSON", k400BadRequest);
        callback(resp);
        return;
    }

    Validator::ErrorCollector validator;

    std::string type = (*json).get("type", "batiment").asString();
    std::string geom_type = (*json).get("geom_type", "POLYGON").asString();
    std::string wkt_geometry = (*json).get("wkt", "").asString();

    // Validation
    if (!Validator::isNotEmpty(type)) {
        validator.addError("type", "Obstacle type is required");
    } else if (!Validator::isValidObstacleType(type)) {
        validator.addError("type", "Type must be: batiment, vegetation, relief");
    }

    if (!Validator::isNotEmpty(geom_type)) {
        validator.addError("geom_type", "Geometry type is required");
    } else if (geom_type != "POINT" && geom_type != "POLYGON" && geom_type != "LINESTRING") {
        validator.addError("geom_type", "Geometry type must be: POINT, POLYGON, or LINESTRING");
    }

    if (!Validator::isNotEmpty(wkt_geometry)) {
        validator.addError("wkt", "WKT geometry is required");
    }

    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    ObstacleModel o;
    o.type = type;
    o.geom_type = geom_type;
    o.wkt_geometry = wkt_geometry;

    ObstacleService::create(o, [callback](const std::string& err) {
        if (err.empty()) {
            Json::Value ret;
            ret["success"] = true;
            ret["message"] = "Obstacle created successfully";
            
            auto now = trantor::Date::now();
            ret["timestamp"] = now.toFormattedString(false);
            
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k201Created);
            callback(resp);
        } else {
            auto errorDetails = ErrorHandler::analyzePostgresError(err);
            ErrorHandler::logError("ObstacleController::create", errorDetails);
            auto resp = ErrorHandler::createErrorResponse(errorDetails);
            callback(resp);
        }
    });
}

// ============================================================================
// 2. READ ALL avec pagination optionnelle
// ============================================================================
void ObstacleController::getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto paginationParams = parsePaginationParams(req);
    
    Validator::ErrorCollector validator;
    if (paginationParams.page < 1) {
        validator.addError("page", "Page number must be at least 1");
    }
    if (paginationParams.pageSize < 1 || paginationParams.pageSize > 100) {
        validator.addError("pageSize", "Page size must be between 1 and 100");
    }
    
    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }
    
    if (paginationParams.usePagination) {
        ObstacleService::getAllPaginated(paginationParams.page, paginationParams.pageSize,
            [callback](const std::vector<ObstacleModel>& list, const PaginationMeta& meta, const std::string& err) {
                if (err.empty()) {
                    Json::Value response;
                    response["success"] = true;
                    
                    Json::Value dataArray(Json::arrayValue);
                    for (const auto& item : list) {
                        dataArray.append(item.toJson());
                    }
                    response["data"] = dataArray;
                    response["pagination"] = meta.toJson();
                    addPaginationLinks(response, meta, "/api/obstacles");
                    
                    auto now = trantor::Date::now();
                    response["timestamp"] = now.toFormattedString(false);
                    
                    auto resp = HttpResponse::newHttpJsonResponse(response);
                    callback(resp);
                } else {
                    auto errorDetails = ErrorHandler::analyzePostgresError(err);
                    ErrorHandler::logError("ObstacleController::getAll (paginated)", errorDetails);
                    auto resp = ErrorHandler::createErrorResponse(errorDetails);
                    callback(resp);
                }
            }
        );
    } else {
        ObstacleService::getAll([callback](const std::vector<ObstacleModel>& list, const std::string& err) {
            if (err.empty()) {
                Json::Value response;
                response["success"] = true;
                
                Json::Value arr(Json::arrayValue);
                for (const auto& item : list) {
                    arr.append(item.toJson());
                }
                response["data"] = arr;
                response["count"] = static_cast<int>(list.size());
                
                auto now = trantor::Date::now();
                response["timestamp"] = now.toFormattedString(false);
                
                auto resp = HttpResponse::newHttpJsonResponse(response);
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("ObstacleController::getAll", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        });
    }
}

// ============================================================================
// 3. READ ONE
// ============================================================================
void ObstacleController::getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    if (!Validator::isNonNegative(id) || id == 0) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid obstacle ID", k400BadRequest);
        callback(resp);
        return;
    }

    ObstacleService::getById(id, [callback, id](const ObstacleModel& o, const std::string& err) {
        if (err.empty()) {
            Json::Value response;
            response["success"] = true;
            response["data"] = o.toJson();
            
            auto now = trantor::Date::now();
            response["timestamp"] = now.toFormattedString(false);
            
            auto resp = HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        } else {
            if (err.find("Not Found") != std::string::npos || err.find("not found") != std::string::npos) {
                auto resp = ErrorHandler::createNotFoundResponse("Obstacle", id);
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("ObstacleController::getById", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        }
    });
}

// ============================================================================
// 4. UPDATE
// ============================================================================
void ObstacleController::update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    auto json = req->getJsonObject();

    if (!json) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid JSON", k400BadRequest);
        callback(resp);
        return;
    }

    if (!Validator::isNonNegative(id) || id == 0) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid obstacle ID", k400BadRequest);
        callback(resp);
        return;
    }

    Validator::ErrorCollector validator;
    ObstacleModel o;
    o.id = id;

    if (json->isMember("type")) {
        std::string type = (*json)["type"].asString();
        if (!Validator::isValidObstacleType(type)) {
            validator.addError("type", "Type must be: batiment, vegetation, relief");
        } else {
            o.type = type;
        }
    }

    if (json->isMember("geom_type")) {
        std::string geom_type = (*json)["geom_type"].asString();
        if (geom_type != "POINT" && geom_type != "POLYGON" && geom_type != "LINESTRING") {
            validator.addError("geom_type", "Geometry type must be: POINT, POLYGON, or LINESTRING");
        } else {
            o.geom_type = geom_type;
        }
    }

    if (json->isMember("wkt")) {
        o.wkt_geometry = (*json)["wkt"].asString();
    }

    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    ObstacleService::update(o, [callback, id](const std::string& err) {
        if (err.empty()) {
            Json::Value ret;
            ret["success"] = true;
            ret["message"] = "Obstacle updated successfully";
            ret["obstacleId"] = id;
            
            auto now = trantor::Date::now();
            ret["timestamp"] = now.toFormattedString(false);
            
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
        } else {
            if (err.find("not found") != std::string::npos || err.find("Not Found") != std::string::npos) {
                auto resp = ErrorHandler::createNotFoundResponse("Obstacle", id);
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("ObstacleController::update", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        }
    });
}

// ============================================================================
// 5. DELETE
// ============================================================================
void ObstacleController::remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    if (!Validator::isNonNegative(id) || id == 0) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid obstacle ID", k400BadRequest);
        callback(resp);
        return;
    }

    ObstacleService::remove(id, [callback, id](const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k204NoContent);
            callback(resp);
        } else {
            if (err.find("not found") != std::string::npos || err.find("Not Found") != std::string::npos) {
                auto resp = ErrorHandler::createNotFoundResponse("Obstacle", id);
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("ObstacleController::remove", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        }
    });
}

// ============================================================================
// 6. GET GEOJSON avec pagination optionnelle
// ============================================================================
void ObstacleController::getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto paginationParams = parsePaginationParams(req);
    
    if (paginationParams.usePagination) {
        ObstacleService::getAllGeoJSONPaginated(paginationParams.page, paginationParams.pageSize,
            [callback](const Json::Value& geojson, const PaginationMeta& meta, const std::string& err) {
                if (err.empty()) {
                    Json::Value response = geojson;
                    response["pagination"] = meta.toJson();
                    addPaginationLinks(response, meta, "/api/obstacles/geojson");
                    
                    auto resp = HttpResponse::newHttpJsonResponse(response);
                    resp->addHeader("Content-Type", "application/geo+json");
                    callback(resp);
                } else {
                    auto errorDetails = ErrorHandler::analyzePostgresError(err);
                    ErrorHandler::logError("ObstacleController::getGeoJSON (paginated)", errorDetails);
                    auto resp = ErrorHandler::createErrorResponse(errorDetails);
                    callback(resp);
                }
            }
        );
    } else {
        ObstacleService::getAllGeoJSON([callback](const Json::Value& geojson, const std::string& err) {
            if (err.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(geojson);
                resp->addHeader("Content-Type", "application/geo+json");
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("ObstacleController::getGeoJSON", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        });
    }
}

// ============================================================================
// 7. GET OBSTACLES BY BOUNDING BOX
// ============================================================================
void ObstacleController::getByBoundingBox(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto bbox = req->getParameter("bbox"); // Format: minLon,minLat,maxLon,maxLat
    auto type = req->getOptionalParameter<std::string>("type"); // Optional filter by type

    // Parse bbox into coordinates (robust)
    std::vector<std::string> coords;
    std::stringstream ss(bbox);
    std::string item;
    while (std::getline(ss, item, ',')) {
        coords.push_back(item);
    }
    if (coords.size() != 4) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid bounding box format. Expected: minLon,minLat,maxLon,maxLat");
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        callback(resp);
        return;
    }

    auto trim = [](const std::string &s) {
        const char* ws = " \t\n\r";
        size_t b = s.find_first_not_of(ws);
        if (b == std::string::npos) return std::string();
        size_t e = s.find_last_not_of(ws);
        return s.substr(b, e - b + 1);
    };

    double minLon, minLat, maxLon, maxLat;
    try {
        minLon = std::stod(trim(coords[0]));
        minLat = std::stod(trim(coords[1]));
        maxLon = std::stod(trim(coords[2]));
        maxLat = std::stod(trim(coords[3]));
    } catch (const std::invalid_argument &e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Bounding box contains invalid number");
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        callback(resp);
        return;
    } catch (const std::out_of_range &e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Bounding box number out of range");
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        callback(resp);
        return;
    }

    Validator::ErrorCollector validatorCoords;
    if (!Validator::isValidLongitude(minLon) || !Validator::isValidLongitude(maxLon)) {
        validatorCoords.addError("longitude", "Longitudes must be between -180 and +180 degrees");
    }
    if (!Validator::isValidLatitude(minLat) || !Validator::isValidLatitude(maxLat)) {
        validatorCoords.addError("latitude", "Latitudes must be between -90 and +90 degrees");
    }
    if (minLat >= maxLat) {
        validatorCoords.addError("bbox", "minLat must be less than maxLat");
    }
    if (minLon >= maxLon) {
        validatorCoords.addError("bbox", "minLon must be less than maxLon");
    }
    if (validatorCoords.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validatorCoords.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    // Query database
    ObstacleService::getByBoundingBox(minLon, minLat, maxLon, maxLat, type, [callback](const Json::Value& geojson, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(geojson);
            resp->addHeader("Content-Type", "application/geo+json");
            callback(resp);
        } else {
            auto errorDetails = ErrorHandler::analyzePostgresError(err);
            ErrorHandler::logError("ObstacleController::getByBoundingBox", errorDetails);
            auto resp = ErrorHandler::createErrorResponse(errorDetails);
            callback(resp);
        }
    });
}