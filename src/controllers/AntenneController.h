#pragma once
#include <drogon/HttpController.h>
#include "../services/AntenneService.h"

using namespace drogon;

class AntenneController : public drogon::HttpController<AntenneController> {
public:
    METHOD_LIST_BEGIN
        // ========== CRUD CLASSIQUE ==========
        ADD_METHOD_TO(AntenneController::create, "/api/antennes", Post);
        ADD_METHOD_TO(AntenneController::getAll, "/api/antennes", Get); // Supporte pagination via query params
        ADD_METHOD_TO(AntenneController::getById, "/api/antennes/{1}", Get);
        ADD_METHOD_TO(AntenneController::update, "/api/antennes/{1}", Put);
        ADD_METHOD_TO(AntenneController::remove, "/api/antennes/{1}", Delete);
        
        // ========== RECHERCHE GÉOGRAPHIQUE ==========
        ADD_METHOD_TO(AntenneController::search, "/api/antennes/search?lat={1}&lon={2}&radius={3}", Get); // Supporte pagination
        
        // ========== GEOJSON POUR LEAFLET ==========
        ADD_METHOD_TO(AntenneController::getGeoJSON, "/api/antennes/geojson", Get); // Supporte pagination
        ADD_METHOD_TO(AntenneController::getGeoJSONInRadius, "/api/antennes/geojson/radius?lat={1}&lon={2}&radius={3}", Get); // Supporte pagination
        ADD_METHOD_TO(AntenneController::getGeoJSONInBBox, "/api/antennes/geojson/bbox?minLat={1}&minLon={2}&maxLat={3}&maxLon={4}", Get);
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
};