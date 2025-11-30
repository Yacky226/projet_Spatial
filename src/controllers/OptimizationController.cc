#include "OptimizationController.h"
#include "../models/OptimizationRequest.h"
#include "../utils/ErrorHandler.h"

void OptimizationController::optimize(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid JSON", k400BadRequest);
        callback(resp);
        return;
    }

    auto request = OptimizationRequest::fromJson(json);

    // 🆕 Validation avec operator_id
    if (request.zone_id <= 0) {
        auto resp = ErrorHandler::createGenericErrorResponse("Invalid zone_id", k400BadRequest);
        callback(resp);
        return;
    }
    
    if (request.operator_id <= 0) {
        auto resp = ErrorHandler::createGenericErrorResponse("operator_id is required and must be > 0", k400BadRequest);
        callback(resp);
        return;
    }
    
    if (request.antennas_count <= 0 || request.antennas_count > 50) {
        auto resp = ErrorHandler::createGenericErrorResponse("antennas_count must be between 1 and 50", k400BadRequest);
        callback(resp);
        return;
    }
    
    if (request.radius <= 0 || request.radius > 10000) {
        auto resp = ErrorHandler::createGenericErrorResponse("radius must be between 1 and 10000 meters", k400BadRequest);
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
                response["strategy"] = "K-Means Clustering (Operator-Specific)";
                
                Json::Value arr(Json::arrayValue);
                for (const auto& item : res) {
                    arr.append(item.toJson());
                }
                response["candidates"] = arr;
                
                auto resp = HttpResponse::newHttpJsonResponse(response);
                callback(resp);
            } else {
                auto resp = ErrorHandler::createGenericErrorResponse(err, k500InternalServerError);
                callback(resp);
            }
        });
    } else {
        OptimizationService::optimizeGreedy(request, [callback](const std::vector<OptimizationResult>& res, const std::string& err) {
            if (err.empty()) {
                Json::Value finalJson;
                finalJson["success"] = true;
                finalJson["strategy"] = "Greedy Coverage Maximization (Operator-Specific)";
                
                Json::Value arr(Json::arrayValue);
                for (const auto& item : res) {
                    arr.append(item.toJson());
                }
                finalJson["candidates"] = arr;

                auto resp = HttpResponse::newHttpJsonResponse(finalJson);
                callback(resp);
            } else {
                auto resp = ErrorHandler::createGenericErrorResponse(err, k500InternalServerError);
                callback(resp);
            }
        });
    }
}