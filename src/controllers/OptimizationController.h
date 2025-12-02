#pragma once
#include <drogon/HttpController.h>
#include "../services/OptimizationService.h"

using namespace drogon;

class OptimizationController : public drogon::HttpController<OptimizationController> {
public:
    METHOD_LIST_BEGIN
        // Une seule méthode qui gère les deux algorithmes
        ADD_METHOD_TO(OptimizationController::optimize, "/api/optimization/optimize", Post);
        ADD_METHOD_TO(OptimizationController::options, "/api/optimization/optimize", Options);
    METHOD_LIST_END

    void optimize(const HttpRequestPtr& req, 
                  std::function<void (const HttpResponsePtr &)> &&callback);
    void options(const HttpRequestPtr& req, 
                 std::function<void (const HttpResponsePtr &)> &&callback);
};