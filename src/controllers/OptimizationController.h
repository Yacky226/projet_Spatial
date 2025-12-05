#pragma once
#include <drogon/HttpController.h>
#include "../services/OptimizationService.h"

using namespace drogon;

class OptimizationController : public drogon::HttpController<OptimizationController> {
public:
    METHOD_LIST_BEGIN
        // Une seule méthode qui gère les deux algorithmes
        ADD_METHOD_TO(OptimizationController::optimize, "/api/optimization/optimize", Post);
        // Ajouter OPTIONS pour CORS preflight
        ADD_METHOD_TO(OptimizationController::handleOptions, "/api/optimization/optimize", Options);
    METHOD_LIST_END

    void optimize(const HttpRequestPtr& req, 
                  std::function<void (const HttpResponsePtr &)> &&callback);
    
    // Gestionnaire pour les requêtes OPTIONS (CORS preflight)
    void handleOptions(const HttpRequestPtr& req, 
                      std::function<void (const HttpResponsePtr &)> &&callback);
};