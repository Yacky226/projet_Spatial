#include "OptimizationController.h"
#include "../models/OptimizationRequest.h"

void OptimizationController::optimize(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    auto request = OptimizationRequest::fromJson(json);

    // Validation
    if (request.zone_id <= 0 || request.antennas_count <= 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid parameters");
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