#include "OptimizationController.h"
#include "../models/OptimizationRequest.h"

// Gestionnaire pour les requêtes OPTIONS (CORS preflight)
void OptimizationController::handleOptions(const HttpRequestPtr& req, 
                                          std::function<void (const HttpResponsePtr &)> &&callback) {
    LOG_INFO << "🔄 CORS preflight request for optimization endpoint";
    
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->addHeader("Access-Control-Allow-Origin", "*");
    resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    resp->addHeader("Access-Control-Max-Age", "86400");
    
    callback(resp);
}

void OptimizationController::optimize(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    LOG_INFO << "🎯 Optimization request received";
    
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    auto request = OptimizationRequest::fromJson(json);
    
    LOG_INFO << "🎯 Optimization params: zone_id=" << (request.zone_id.has_value() ? std::to_string(request.zone_id.value()) : "none")
             << ", antennas=" << request.antennas_count
             << ", radius=" << request.radius;

    // Validation: zone_id XOR bbox_wkt
    if (!request.isValid()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid parameters: must provide either zone_id OR bbox_wkt (not both, not neither)");
        callback(resp);
        return;
    }
    
    // Validation: antennas_count
    if (request.antennas_count <= 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid parameters: antennas_count must be positive");
        callback(resp);
        return;
    }

    // Choisir l'algorithme
    std::string algorithm = (*json).get("algorithm", "greedy").asString();
    
    if (algorithm == "kmeans") {
        OptimizationService::optimizeKMeans(request, [callback](const std::vector<OptimizationResult>& res, const std::string& err) {
            if (err.empty()) {
                Json::Value response;
                response["success"] = true;
                response["strategy"] = "K-Means Clustering";
                
                Json::Value arr(Json::arrayValue);
                for (const auto& item : res) {
                    arr.append(item.toJson());
                }
                response["candidates"] = arr;
                
                auto resp = HttpResponse::newHttpJsonResponse(response);
                callback(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(err);
                callback(resp);
            }
        });
    } else {
        // Algorithme greedy existant
        OptimizationService::optimizeGreedy(request, [callback](const std::vector<OptimizationResult>& res, const std::string& err) {
            if (err.empty()) {
                Json::Value arr(Json::arrayValue);
                for (const auto& item : res) {
                    arr.append(item.toJson());
                }
                
                Json::Value finalJson;
                finalJson["success"] = true;
                finalJson["strategy"] = "Greedy Coverage Maximization";
                finalJson["candidates"] = arr;

                auto resp = HttpResponse::newHttpJsonResponse(finalJson);
                callback(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(err);
                callback(resp);
            }
        });
    }
}