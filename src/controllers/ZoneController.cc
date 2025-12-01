#include "ZoneController.h"
#include "../services/CacheService.h"

// 1. Create
void ZoneController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) { /* 400 Bad Request */ return; }

    ZoneModel z;
    z.name = (*json).get("name", "").asString();
    z.type = (*json).get("type", "coverage").asString();
    z.density = (*json).get("density", 0.0).asDouble();
    z.wkt_geometry = (*json).get("wkt", "").asString(); // Ex: "POLYGON((...))"
    z.parent_id = (*json).get("parent_id", 0).asInt();

    ZoneService::create(z, [callback](const std::string& err){
        if(err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k201Created);
            resp->setBody("Zone created");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 2. Read All
void ZoneController::getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    ZoneService::getAll([callback](const std::vector<ZoneModel>& list, const std::string& err) {
        if(err.empty()){
            Json::Value arr(Json::arrayValue);
            for(auto &z : list) arr.append(z.toJson());
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            callback(resp);
        } else {
            /* 500 Error */
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 3. Read One
void ZoneController::getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    ZoneService::getById(id, [callback](const ZoneModel& z, const std::string& err){
        if(err.empty()){
            auto resp = HttpResponse::newHttpJsonResponse(z.toJson());
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            callback(resp);
        }
    });
}

// 3.5. Read By Type
void ZoneController::getByType(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& type) {
    ZoneService::getByType(type, [callback](const std::vector<ZoneModel>& list, const std::string& err) {
        if(err.empty()){
            Json::Value arr(Json::arrayValue);
            for(auto &z : list) arr.append(z.toJson());
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 3.6. Read By Type Simplified (Sprint 2 - Optimization)
/**
 * Endpoint pour récupérer les zones par type avec simplification géométrique
 * 
 * Route: GET /api/zones/type/{type}/simplified?zoom={zoom}
 * 
 * Paramètres:
 * - type (path): Type de zone (country, region, province, commune, etc.)
 * - zoom (query): Niveau de zoom Leaflet (0-18) pour adapter la simplification
 * 
 * Réponse: JSON array de zones avec géométries simplifiées selon le zoom
 * 
 * Exemple: GET /api/zones/type/commune/simplified?zoom=10
 * 
 * Avantage: Réduit la taille des données de ~50% pour améliorer les performances
 * de rendu sur la carte, surtout à petits zooms (vue monde/pays)
 */
void ZoneController::getByTypeSimplified(
    const HttpRequestPtr& req, 
    std::function<void (const HttpResponsePtr &)> &&callback, 
    const std::string& type, 
    int zoom) 
{
    // Validation du zoom (0-18 pour Leaflet standard)
    if (zoom < 0 || zoom > 18) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(R"({"error": "Invalid zoom level. Must be between 0 and 18"})");
        callback(resp);
        return;
    }
    
    // Sprint 3: Vérifier cache Redis
    std::string cacheKey = "type:" + type + ":zoom:" + std::to_string(zoom);
    auto cached = CacheService::getInstance().getCachedZones(cacheKey);
    if (cached) {
        LOG_INFO << "✅ Cache HIT (zones): " << cacheKey;
        auto resp = HttpResponse::newHttpJsonResponse(*cached);
        resp->addHeader("X-Cache", "HIT");
        callback(resp);
        return;
    }
    
    LOG_INFO << "❌ Cache MISS (zones): " << cacheKey;
    
    // Appel au service avec simplification
    ZoneService::getByTypeSimplified(type, zoom, 
        [callback, zoom, cacheKey](const std::vector<ZoneModel>& list, const std::string& err) {
            if(err.empty()){
                Json::Value arr(Json::arrayValue);
                for(auto &z : list) {
                    arr.append(z.toJson());
                }
                
                // Sprint 3: Mettre en cache Redis (TTL 1h)
                CacheService::getInstance().cacheZones(cacheKey, arr);
                LOG_INFO << "💾 Cached (zones): " << cacheKey;
                
                auto resp = HttpResponse::newHttpJsonResponse(arr);
                resp->addHeader("X-Cache", "MISS");
                resp->addHeader("Cache-Control", "public, max-age=3600");
                
                callback(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(err);
                callback(resp);
            }
        }
    );
}

// 4. Update
void ZoneController::update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    auto json = req->getJsonObject();
    if(!json) { /* 400 */ return; }

    ZoneModel z;
    z.id = id;
    z.name = (*json).get("name", "").asString();
    z.type = (*json).get("type", "coverage").asString();
    z.density = (*json).get("density", 0.0).asDouble();
    z.wkt_geometry = (*json).get("wkt", "").asString();
    z.parent_id = (*json).get("parent_id", 0).asInt();

    ZoneService::update(z, [callback, z](const std::string& err){
        if(err.empty()){
            // Sprint 3: Invalider cache après update
            CacheService::getInstance().invalidateZonesByType(z.type);
            
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setBody("Zone updated");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 5. Delete
void ZoneController::remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    ZoneService::remove(id, [callback](const std::string& err){
        if(err.empty()){
            // Sprint 3: Invalider tout le cache zones après delete
            CacheService::getInstance().delPattern("zones:*");
            
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k204NoContent);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

void ZoneController::getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    ZoneService::getAllGeoJSON([callback](const Json::Value& json, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(json);
            callback(resp); // Renvoie la FeatureCollection complète
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}



void ZoneController::getWhiteZones(const HttpRequestPtr& req, 
                                  std::function<void (const HttpResponsePtr &)> &&callback, 
                                  int zone_id, int operator_id) {
    
    // Validation basique
    if (zone_id <= 0 || operator_id <= 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(R"({"error": "Invalid zone_id or operator_id"})");
        callback(resp);
        return;
    }

    ZoneService::getWhiteZones(zone_id, operator_id, 
        [callback](const Json::Value& geojson, const std::string& err) {
            if (err.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(geojson);
                // Le calcul est lourd, on encourage le navigateur à le mettre en cache pour 1 minute
                resp->addHeader("Cache-Control", "public, max-age=60");
                callback(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(R"({"error": "Calculation failed", "details": ")" + err + R"("})");
                callback(resp);
            }
        }
    );
}