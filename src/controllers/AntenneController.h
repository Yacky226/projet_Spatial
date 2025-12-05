#pragma once
#include <drogon/HttpController.h>
#include "../services/AntenneService.h"

using namespace drogon;

class AntenneController : public drogon::HttpController<AntenneController> {
public:
    METHOD_LIST_BEGIN
        // NOUVEAU : Clustering backend optimisé (Sprint 1)
        ADD_METHOD_TO(AntenneController::getClusteredAntennas, "/api/antennas/clustered?minLat={1}&minLon={2}&maxLat={3}&maxLon={4}&zoom={5}", Get);
        
        // NOUVEAU : Simplified Coverage (Sprint 4 Performance + Filtres)
        ADD_METHOD_TO(AntenneController::getSimplifiedCoverage, "/api/antennas/coverage/simplified?minLat={1}&minLon={2}&maxLat={3}&maxLon={4}&zoom={5}", Get);
    METHOD_LIST_END

    // ========== CLUSTERING (Sprint 1 Optimization) ==========
    // Clustering backend pour remplacer le clustering client-side
    // Utilise ST_SnapToGrid pour grouper les antennes proches
    // Supporte filtres optionnels: status, technology, operator_id
    void getClusteredAntennas(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback,
                             double minLat, double minLon, double maxLat, double maxLon, int zoom);
    
    // ========== SIMPLIFIED COVERAGE (Sprint 4 Performance) ==========
    // Couverture simplifiée ultra-optimisée pour navigation fluide
    // Utilise ST_Union + ST_Simplify + cache Redis
    void getSimplifiedCoverage(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback,
                              double minLat, double minLon, double maxLat, double maxLon, int zoom);
};