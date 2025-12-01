#pragma once
#include <drogon/HttpController.h>
#include "../services/AntenneService.h"

using namespace drogon;

class AntenneController : public drogon::HttpController<AntenneController> {
public:
    METHOD_LIST_BEGIN
        // ========== CRUD CLASSIQUE ==========
        ADD_METHOD_TO(AntenneController::create, "/api/antennas", Post);
        ADD_METHOD_TO(AntenneController::getAll, "/api/antennas", Get); // Supporte pagination via query params
        ADD_METHOD_TO(AntenneController::getById, "/api/antennas/{1}", Get);
        ADD_METHOD_TO(AntenneController::update, "/api/antennas/{1}", Put);
        ADD_METHOD_TO(AntenneController::remove, "/api/antennas/{1}", Delete);
        
        // ========== RECHERCHE GÉOGRAPHIQUE ==========
        ADD_METHOD_TO(AntenneController::search, "/api/antennas/search?lat={1}&lon={2}&radius={3}", Get); // Supporte pagination
        
        // ========== GEOJSON POUR LEAFLET ==========
        ADD_METHOD_TO(AntenneController::getGeoJSON, "/api/antennas/geojson", Get); // Supporte pagination
        ADD_METHOD_TO(AntenneController::getGeoJSONInRadius, "/api/antennas/geojson/radius?lat={1}&lon={2}&radius={3}", Get); // Supporte pagination
        ADD_METHOD_TO(AntenneController::getGeoJSONInBBox, "/api/antennas/geojson/bbox?minLat={1}&minLon={2}&maxLat={3}&maxLon={4}", Get);
       ADD_METHOD_TO(AntenneController::getCoverage, "/api/coverage/operator/{1}?minLat={2}&minLon={3}&maxLat={4}&maxLon={5}", Get);
        
        // NOUVEAU : Voronoi diagram
        ADD_METHOD_TO(AntenneController::getVoronoi, "/api/antennas/voronoi", Get);
        
        // NOUVEAU : Clustering backend optimisé (Sprint 1)
        ADD_METHOD_TO(AntenneController::getClusteredAntennas, "/api/antennas/clustered?minLat={1}&minLon={2}&maxLat={3}&maxLon={4}&zoom={5}", Get);
        
        // NOUVEAU : Simplified Coverage (Sprint 4 Performance + Filtres)
        ADD_METHOD_TO(AntenneController::getSimplifiedCoverage, "/api/antennas/coverage/simplified?minLat={1}&minLon={2}&maxLat={3}&maxLon={4}&zoom={5}", Get);
    METHOD_LIST_END


    // ========== CRUD ==========
    void create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);

    // ========== RECHERCHE ==========
    void search(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, 
                double lat, double lon, double radius);
    
    // ========== GEOJSON ==========
    void getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getGeoJSONInRadius(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback,
                           double lat, double lon, double radius);
    void getGeoJSONInBBox(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback,
                         double minLat, double minLon, double maxLat, double maxLon);
    
   void getCoverage(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback,
                     int operator_id, double minLat, double minLon, double maxLat, double maxLon);
    void getVoronoi(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    
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