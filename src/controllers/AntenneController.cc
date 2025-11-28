#include "AntenneController.h"
#include "../utils/Validator.h"
#include "../utils/ErrorHandler.h"
#include "../utils/PaginationHelper.h"
#include <drogon/HttpResponse.h>

using namespace drogon;

// ============================================================================
// 1. CREATE avec validation complète + gestion d'erreurs
// ============================================================================
void AntenneController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    auto resp = HttpResponse::newHttpResponse();

    if (!json) {
        resp->setStatusCode(k400BadRequest);
        resp->setBody(R"({"success": false, "error": "Invalid JSON"})");
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    Validator::ErrorCollector validator;

    double latitude = (*json).get("latitude", 0.0).asDouble();
    double longitude = (*json).get("longitude", 0.0).asDouble();
    double coverage_radius = (*json).get("coverage_radius", 1.0).asDouble();
    std::string status = (*json).get("status", "active").asString();
    std::string technology = (*json).get("technology", "4G").asString();
    std::string installation_date = (*json).get("installation_date", "").asString();
    int operator_id = (*json).get("operator_id", 0).asInt();

    // VALIDATION COMPLÈTE
    if (!Validator::isValidLatitude(latitude)) {
        validator.addError("latitude", "Latitude must be between -90 and +90 degrees");
    }
    if (!Validator::isValidLongitude(longitude)) {
        validator.addError("longitude", "Longitude must be between -180 and +180 degrees");
    }
    if (!Validator::isPositive(coverage_radius)) {
        validator.addError("coverage_radius", "Coverage radius must be positive (in meters)");
    } else if (coverage_radius > 50000) {
        validator.addError("coverage_radius", "Coverage radius cannot exceed 50,000 meters (50km)");
    }
    if (!Validator::isNotEmpty(status)) {
        validator.addError("status", "Status is required");
    } else if (!Validator::isValidStatus(status)) {
        validator.addError("status", "Status must be one of: active, inactive, maintenance, planned");
    }
    if (!Validator::isNotEmpty(technology)) {
        validator.addError("technology", "Technology is required");
    } else if (!Validator::isValidTechnology(technology)) {
        validator.addError("technology", "Technology must be one of: 4G, 5G, 5G-SA, 5G-NSA");
    }
    if (!Validator::isNonNegative(operator_id) || operator_id == 0) {
        validator.addError("operator_id", "Operator ID is required and must be a positive integer");
    }

    if (validator.hasErrors()) {
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    Antenna a;
    a.latitude = latitude;
    a.longitude = longitude;
    a.coverage_radius = coverage_radius;
    a.status = status;
    a.technology = technology;
    a.installation_date = installation_date;
    a.operator_id = operator_id;

    AntenneService::create(a, [callback](const std::string& err) {
        if (err.empty()) {
            Json::Value ret;
            ret["success"] = true;
            ret["message"] = "Antenna created successfully";
            
            auto now = trantor::Date::now();
            ret["timestamp"] = now.toFormattedString(false);
            
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k201Created);
            callback(resp);
        } else {
            auto errorDetails = ErrorHandler::analyzePostgresError(err);
            ErrorHandler::logError("AntenneController::create", errorDetails);
            auto resp = ErrorHandler::createErrorResponse(errorDetails);
            callback(resp);
        }
    });
}

// ============================================================================
// 2. READ ALL avec PAGINATION OPTIONNELLE
// ============================================================================
void AntenneController::getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto paginationParams = parsePaginationParams(req);
    
    // Validation des paramètres de pagination
    Validator::ErrorCollector validator;
    
    if (paginationParams.page < 1) {
        validator.addError("page", "Page number must be at least 1");
    }
    
    // Pour la pagination explicite (avec paramètre page), limiter à 100
    // Pour la récupération en masse (pageSize sans page), permettre jusqu'à 10000
    int maxPageSize = paginationParams.page > 1 ? 100 : 10000;
    if (paginationParams.pageSize < 1 || paginationParams.pageSize > maxPageSize) {
        validator.addError("pageSize", "Page size must be between 1 and " + std::to_string(maxPageSize));
    }
    
    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }
    
    // Si pagination explicite demandée (paramètre page fourni)
    if (paginationParams.page > 1) {
        AntenneService::getAllPaginated(paginationParams.page, paginationParams.pageSize,
            [callback, paginationParams](const std::vector<Antenna>& list, const PaginationMeta& meta, const std::string& err) {
                if (err.empty()) {
                    Json::Value response;
                    response["success"] = true;
                    
                    Json::Value dataArray(Json::arrayValue);
                    for (const auto& item : list) {
                        dataArray.append(item.toJson());
                    }
                    response["data"] = dataArray;
                    
                    // Ajouter les métadonnées de pagination
                    response["pagination"] = meta.toJson();
                    addPaginationLinks(response, meta, "/api/antennes");
                    
                    auto now = trantor::Date::now();
                    response["timestamp"] = now.toFormattedString(false);
                    
                    auto resp = HttpResponse::newHttpJsonResponse(response);
                    callback(resp);
                } else {
                    auto errorDetails = ErrorHandler::analyzePostgresError(err);
                    ErrorHandler::logError("AntenneController::getAll (paginated)", errorDetails);
                    auto resp = ErrorHandler::createErrorResponse(errorDetails);
                    callback(resp);
                }
            }
        );
    } else {
        // Sans pagination (rétrocompatibilité) - récupération en masse
        AntenneService::getAll([callback](const std::vector<Antenna>& list, const std::string& err) {
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
                ErrorHandler::logError("AntenneController::getAll", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        });
    }
}

// ============================================================================
// 3. READ ONE avec validation ID + gestion d'erreurs
// ============================================================================
void AntenneController::getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    if (!Validator::isNonNegative(id) || id == 0) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid antenna ID", k400BadRequest);
        callback(resp);
        return;
    }

    AntenneService::getById(id, [callback, id](const Antenna& a, const std::string& err) {
        if (err.empty()) {
            Json::Value response;
            response["success"] = true;
            response["data"] = a.toJson();
            
            auto now = trantor::Date::now();
            response["timestamp"] = now.toFormattedString(false);
            
            auto resp = HttpResponse::newHttpJsonResponse(response);
            callback(resp);
        } else {
            if (err.find("Not Found") != std::string::npos || err.find("not found") != std::string::npos) {
                auto resp = ErrorHandler::createNotFoundResponse("Antenna", id);
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::getById", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        }
    });
}

// ============================================================================
// 4. UPDATE avec validation partielle + gestion d'erreurs
// ============================================================================
void AntenneController::update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    auto json = req->getJsonObject();

    if (!json) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid JSON", k400BadRequest);
        callback(resp);
        return;
    }

    if (!Validator::isNonNegative(id) || id == 0) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid antenna ID", k400BadRequest);
        callback(resp);
        return;
    }

    Validator::ErrorCollector validator;
    Antenna a;
    a.id = id;

    if (json->isMember("latitude")) {
        double latitude = (*json)["latitude"].asDouble();
        if (!Validator::isValidLatitude(latitude)) {
            validator.addError("latitude", "Latitude must be between -90 and +90 degrees");
        } else {
            a.latitude = latitude;
        }
    }

    if (json->isMember("longitude")) {
        double longitude = (*json)["longitude"].asDouble();
        if (!Validator::isValidLongitude(longitude)) {
            validator.addError("longitude", "Longitude must be between -180 and +180 degrees");
        } else {
            a.longitude = longitude;
        }
    }

    if (json->isMember("coverage_radius")) {
        double radius = (*json)["coverage_radius"].asDouble();
        if (!Validator::isPositive(radius)) {
            validator.addError("coverage_radius", "Coverage radius must be positive");
        } else if (radius > 50000) {
            validator.addError("coverage_radius", "Coverage radius cannot exceed 50,000 meters");
        } else {
            a.coverage_radius = radius;
        }
    }

    if (json->isMember("status")) {
        std::string status = (*json)["status"].asString();
        if (!Validator::isValidStatus(status)) {
            validator.addError("status", "Status must be: active, inactive, maintenance, or planned");
        } else {
            a.status = status;
        }
    }

    if (json->isMember("technology")) {
        std::string tech = (*json)["technology"].asString();
        if (!Validator::isValidTechnology(tech)) {
            validator.addError("technology", "Technology must be: 4G, 5G, 5G-SA, or 5G-NSA");
        } else {
            a.technology = tech;
        }
    }

    if (json->isMember("installation_date")) {
        a.installation_date = (*json)["installation_date"].asString();
    }

    if (json->isMember("operator_id")) {
        int op_id = (*json)["operator_id"].asInt();
        if (!Validator::isNonNegative(op_id) || op_id == 0) {
            validator.addError("operator_id", "Operator ID must be a positive integer");
        } else {
            a.operator_id = op_id;
        }
    }

    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    AntenneService::update(a, [callback, id](const std::string& err) {
        if (err.empty()) {
            Json::Value ret;
            ret["success"] = true;
            ret["message"] = "Antenna updated successfully";
            ret["antennaId"] = id;
            
            auto now = trantor::Date::now();
            ret["timestamp"] = now.toFormattedString(false);
            
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k200OK);
            callback(resp);
        } else {
            if (err.find("not found") != std::string::npos || err.find("Not Found") != std::string::npos) {
                auto resp = ErrorHandler::createNotFoundResponse("Antenna", id);
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::update", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        }
    });
}

// ============================================================================
// 5. DELETE avec validation ID + gestion d'erreurs améliorée
// ============================================================================
void AntenneController::remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    if (!Validator::isNonNegative(id) || id == 0) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid antenna ID", k400BadRequest);
        callback(resp);
        return;
    }

    AntenneService::remove(id, [callback, id](const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k204NoContent);
            callback(resp);
        } else {
            if (err.find("not found") != std::string::npos || err.find("Not Found") != std::string::npos) {
                auto resp = ErrorHandler::createNotFoundResponse("Antenna", id);
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::remove", errorDetails);
                
                if (errorDetails.type == ErrorHandler::ErrorType::FOREIGN_KEY_VIOLATION) {
                    ErrorHandler::ErrorDetails customError = errorDetails;
                    customError.userMessage = "Cannot delete antenna: it is still referenced by zones or other entities. Please remove those references first.";
                    auto resp = ErrorHandler::createErrorResponse(customError);
                    callback(resp);
                } else {
                    auto resp = ErrorHandler::createErrorResponse(errorDetails);
                    callback(resp);
                }
            }
        }
    });
}

// ============================================================================
// 6. SEARCH IN RADIUS avec PAGINATION OPTIONNELLE
// ============================================================================
void AntenneController::search(const HttpRequestPtr& req, 
                               std::function<void (const HttpResponsePtr &)> &&callback, 
                               double lat, double lon, double radius) {
    
    Validator::ErrorCollector validator;

    if (!Validator::isValidLatitude(lat)) {
        validator.addError("latitude", "Latitude must be between -90 and +90 degrees");
    }
    if (!Validator::isValidLongitude(lon)) {
        validator.addError("longitude", "Longitude must be between -180 and +180 degrees");
    }
    if (!Validator::isPositive(radius)) {
        validator.addError("radius", "Radius must be positive (in meters)");
    } else if (radius > 100000) {
        validator.addError("radius", "Search radius cannot exceed 100,000 meters (100km)");
    }

    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    auto paginationParams = parsePaginationParams(req);
    
    if (paginationParams.usePagination) {
        // Recherche paginée
        AntenneService::getInRadiusPaginated(lat, lon, radius, paginationParams.page, paginationParams.pageSize,
            [callback, lat, lon, radius](const std::vector<Antenna>& list, const PaginationMeta& meta, const std::string& err) {
                if (err.empty()) {
                    Json::Value response;
                    response["success"] = true;
                    
                    Json::Value dataArray(Json::arrayValue);
                    for (const auto& item : list) {
                        dataArray.append(item.toJson());
                    }
                    response["data"] = dataArray;
                    
                    Json::Value searchParams;
                    searchParams["latitude"] = lat;
                    searchParams["longitude"] = lon;
                    searchParams["radius"] = radius;
                    response["searchParams"] = searchParams;
                    
                    response["pagination"] = meta.toJson();
                    
                    std::string baseUrl = "/api/antennes/search?lat=" + std::to_string(lat) + 
                                         "&lon=" + std::to_string(lon) + "&radius=" + std::to_string(radius);
                    addPaginationLinks(response, meta, baseUrl);
                    
                    auto now = trantor::Date::now();
                    response["timestamp"] = now.toFormattedString(false);
                    
                    auto resp = HttpResponse::newHttpJsonResponse(response);
                    callback(resp);
                } else {
                    auto errorDetails = ErrorHandler::analyzePostgresError(err);
                    ErrorHandler::logError("AntenneController::search (paginated)", errorDetails);
                    auto resp = ErrorHandler::createErrorResponse(errorDetails);
                    callback(resp);
                }
            }
        );
    } else {
        // Recherche sans pagination
        AntenneService::getInRadius(lat, lon, radius, [callback, lat, lon, radius](const std::vector<Antenna>& list, const std::string& err) {
            if (err.empty()) {
                Json::Value response;
                response["success"] = true;
                
                Json::Value arr(Json::arrayValue);
                for (const auto& item : list) {
                    arr.append(item.toJson());
                }
                response["data"] = arr;
                response["count"] = static_cast<int>(list.size());
                
                Json::Value searchParams;
                searchParams["latitude"] = lat;
                searchParams["longitude"] = lon;
                searchParams["radius"] = radius;
                response["searchParams"] = searchParams;
                
                auto now = trantor::Date::now();
                response["timestamp"] = now.toFormattedString(false);
                
                auto resp = HttpResponse::newHttpJsonResponse(response);
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::search", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        });
    }
}

// ============================================================================
// 7. GET GEOJSON avec PAGINATION OPTIONNELLE
// ============================================================================
void AntenneController::getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto paginationParams = parsePaginationParams(req);
    
    if (paginationParams.usePagination) {
        // GeoJSON paginé
        AntenneService::getAllGeoJSONPaginated(paginationParams.page, paginationParams.pageSize,
            [callback, paginationParams](const Json::Value& geojson, const PaginationMeta& meta, const std::string& err) {
                if (err.empty()) {
                    Json::Value response = geojson;
                    response["pagination"] = meta.toJson();
                    
                    addPaginationLinks(response, meta, "/api/antennes/geojson");
                    
                    auto resp = HttpResponse::newHttpJsonResponse(response);
                    resp->addHeader("Content-Type", "application/geo+json");
                    callback(resp);
                } else {
                    auto errorDetails = ErrorHandler::analyzePostgresError(err);
                    ErrorHandler::logError("AntenneController::getGeoJSON (paginated)", errorDetails);
                    auto resp = ErrorHandler::createErrorResponse(errorDetails);
                    callback(resp);
                }
            }
        );
    } else {
        // GeoJSON complet
        AntenneService::getAllGeoJSON([callback](const Json::Value& geojson, const std::string& err) {
            if (err.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(geojson);
                resp->addHeader("Content-Type", "application/geo+json");
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::getGeoJSON", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        });
    }
}

// ============================================================================
// 8. GET GEOJSON IN RADIUS avec PAGINATION OPTIONNELLE
// ============================================================================
void AntenneController::getGeoJSONInRadius(const HttpRequestPtr& req, 
                                          std::function<void (const HttpResponsePtr &)> &&callback,
                                          double lat, double lon, double radius) {
    
    Validator::ErrorCollector validator;

    if (!Validator::isValidLatitude(lat)) {
        validator.addError("latitude", "Latitude must be between -90 and +90 degrees");
    }
    if (!Validator::isValidLongitude(lon)) {
        validator.addError("longitude", "Longitude must be between -180 and +180 degrees");
    }
    if (!Validator::isPositive(radius)) {
        validator.addError("radius", "Radius must be positive (in meters)");
    } else if (radius > 100000) {
        validator.addError("radius", "Search radius cannot exceed 100,000 meters (100km)");
    }

    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    auto paginationParams = parsePaginationParams(req);
    
    if (paginationParams.usePagination) {
        // GeoJSON radius paginé
        AntenneService::getGeoJSONInRadiusPaginated(lat, lon, radius, paginationParams.page, paginationParams.pageSize,
            [callback, lat, lon, radius](const Json::Value& geojson, const PaginationMeta& meta, const std::string& err) {
                if (err.empty()) {
                    Json::Value response = geojson;
                    response["pagination"] = meta.toJson();
                    
                    std::string baseUrl = "/api/antennes/geojson/radius?lat=" + std::to_string(lat) + 
                                         "&lon=" + std::to_string(lon) + "&radius=" + std::to_string(radius);
                    addPaginationLinks(response, meta, baseUrl);
                    
                    auto resp = HttpResponse::newHttpJsonResponse(response);
                    resp->addHeader("Content-Type", "application/geo+json");
                    callback(resp);
                } else {
                    auto errorDetails = ErrorHandler::analyzePostgresError(err);
                    ErrorHandler::logError("AntenneController::getGeoJSONInRadius (paginated)", errorDetails);
                    auto resp = ErrorHandler::createErrorResponse(errorDetails);
                    callback(resp);
                }
            }
        );
    } else {
        // GeoJSON radius complet
        AntenneService::getGeoJSONInRadius(lat, lon, radius, [callback](const Json::Value& geojson, const std::string& err) {
            if (err.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(geojson);
                resp->addHeader("Content-Type", "application/geo+json");
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::getGeoJSONInRadius", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        });
    }
}

// ============================================================================
// 9. GET GEOJSON IN BBOX (pour Leaflet viewport)
// ============================================================================
void AntenneController::getGeoJSONInBBox(const HttpRequestPtr& req, 
                                        std::function<void (const HttpResponsePtr &)> &&callback,
                                        double minLat, double minLon, double maxLat, double maxLon) {
    
    Validator::ErrorCollector validator;

    if (!Validator::isValidLatitude(minLat) || !Validator::isValidLatitude(maxLat)) {
        validator.addError("latitude", "Latitudes must be between -90 and +90 degrees");
    }
    if (!Validator::isValidLongitude(minLon) || !Validator::isValidLongitude(maxLon)) {
        validator.addError("longitude", "Longitudes must be between -180 and +180 degrees");
    }
    if (minLat >= maxLat) {
        validator.addError("bbox", "minLat must be less than maxLat");
    }
    if (minLon >= maxLon) {
        validator.addError("bbox", "minLon must be less than maxLon");
    }

    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    AntenneService::getGeoJSONInBBox(minLat, minLon, maxLat, maxLon, 
        [callback](const Json::Value& geojson, const std::string& err) {
            if (err.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(geojson);
                resp->addHeader("Content-Type", "application/geo+json");
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::getGeoJSONInBBox", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        }
    );
}

// ============================================================================
// 10. GET COVERAGE BY OPERATOR
// ============================================================================
void AntenneController::getCoverage(const HttpRequestPtr& req, 
                                   std::function<void (const HttpResponsePtr &)> &&callback,
                                   int operator_id, double minLat, double minLon, double maxLat, double maxLon) {
    
    Validator::ErrorCollector validator;

    if (!Validator::isNonNegative(operator_id) || operator_id == 0) {
        validator.addError("operator_id", "Invalid operator ID");
    }
    if (!Validator::isValidLatitude(minLat) || !Validator::isValidLatitude(maxLat)) {
        validator.addError("latitude", "Latitudes must be between -90 and +90 degrees");
    }
    if (!Validator::isValidLongitude(minLon) || !Validator::isValidLongitude(maxLon)) {
        validator.addError("longitude", "Longitudes must be between -180 and +180 degrees");
    }
    // Correction simple si l'utilisateur inverse min/max
    if (minLat > maxLat) std::swap(minLat, maxLat);
    if (minLon > maxLon) std::swap(minLon, maxLon);

    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    AntenneService::getCoverageByOperator(operator_id, minLat, minLon, maxLat, maxLon, 
        [callback](const Json::Value& geojson, const std::string& err) {
            if (err.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(geojson);
                resp->addHeader("Content-Type", "application/geo+json");
                // On ajoute un header pour indiquer au front de mettre en cache ce calcul lourd
                resp->addHeader("Cache-Control", "public, max-age=300"); 
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::getCoverage", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        }
    );
}

// ============================================================================
// NOUVEAU : GET VORONOI DIAGRAM
// ============================================================================
void AntenneController::getVoronoi(const HttpRequestPtr& req, 
                                   std::function<void (const HttpResponsePtr &)> &&callback) {
    AntenneService::getVoronoiDiagram([callback](const Json::Value& geojson, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(geojson);
            resp->addHeader("Content-Type", "application/geo+json");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}