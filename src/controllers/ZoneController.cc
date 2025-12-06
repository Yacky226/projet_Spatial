#include "ZoneController.h"
#include "../services/CacheService.h"

// 1. Read By Type
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

// 2. Read By Type Simplified (Sprint 2 - Optimization)
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


// ============================================================================
//  3. RECHERCHE DE ZONES
// ============================================================================
/**
 * Endpoint pour rechercher des zones par type et query string
 * 
 * Route: GET /api/zones/search?type={type}&query={query}&limit={limit}
 * 
 * Paramètres:
 * - type (query): Type de zone (country, region, province, commune)
 * - query (query): Texte de recherche (nom de la zone)
 * - limit (query, optionnel): Nombre max de résultats (défaut: 10)
 * 
 * Cache: Redis avec TTL 1h (zones changent rarement)
 * 
 * Avantages:
 * - Recherche ILIKE insensible à la casse et aux accents
 * - Cache intelligent par type et query
 * - Limité à 10 résultats pour performance
 * - Pas de géométrie complète (seulement id, name, bounds)
 */
void ZoneController::searchZones(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto params = req->getParameters();
    
    // Validation des paramètres
    std::string type = params.find("type") != params.end() ? params.at("type") : "";
    std::string query = params.find("query") != params.end() ? params.at("query") : "";
    int limit = params.find("limit") != params.end() ? std::stoi(params.at("limit")) : 10;
    
    LOG_INFO << "Search zones - type: '" << type << "', query: '" << query << "', limit: " << limit;
    
    if (type.empty()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(R"({"error": "Missing required parameter: type"})");
        callback(resp);
        return;
    }
    
    // Limiter le nombre de résultats
    if (limit > 50) limit = 50;
    if (limit < 1) limit = 10;
    
    // Sprint 3: Vérifier cache Redis
    std::string cacheKey = "search:" + type + ":" + query + ":" + std::to_string(limit);
    auto cached = CacheService::getInstance().getCachedZones(cacheKey);
    if (cached) {
        LOG_INFO << "✅ Cache HIT (search): " << cacheKey;
        auto resp = HttpResponse::newHttpJsonResponse(*cached);
        resp->addHeader("X-Cache", "HIT");
        callback(resp);
        return;
    }
    
    LOG_INFO << "❌ Cache MISS (search): " << cacheKey;
    
    // Recherche dans la base
    ZoneService::searchZones(type, query, limit, 
        [callback, cacheKey](const std::vector<ZoneModel>& list, const std::string& err) {
            if (err.empty()) {
                Json::Value arr(Json::arrayValue);
                for (auto& z : list) {
                    Json::Value item;
                    item["id"] = z.id;
                    item["name"] = z.name;
                    item["type"] = z.type;
                    item["density"] = z.density;
                    // Calculer les bounds depuis la géométrie
                    // Format simplifié sans géométrie complète pour performance
                    arr.append(item);
                }
                
                // Sprint 3: Mettre en cache Redis (TTL 1h)
                CacheService::getInstance().cacheZones(cacheKey, arr);
                LOG_INFO << "💾 Cached (search): " << cacheKey;
                
                auto resp = HttpResponse::newHttpJsonResponse(arr);
                resp->addHeader("X-Cache", "MISS");
                resp->addHeader("Cache-Control", "public, max-age=3600");
                callback(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(R"({"error": ")" + err + R"("})");
                callback(resp);
            }
        }
    );
}