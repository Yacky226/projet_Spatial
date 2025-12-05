#include "AntenneController.h"
#include "../utils/Validator.h"
#include "../utils/ErrorHandler.h"
#include "../services/CacheService.h"
#include <drogon/HttpResponse.h>
#include <thread>
#include <chrono>

using namespace drogon;


// ============================================================================
// 10. GET CLUSTERED ANTENNAS (Sprint 1 Optimization)
// ============================================================================
/**
 * Endpoint de clustering backend pour remplacer le clustering client-side
 * 
 * Objectif: Réduire la charge frontend en déléguant le clustering au backend
 * Technique: Utilise ST_SnapToGrid de PostGIS pour grouper les antennes proches
 * 
 * Paramètres obligatoires:
 *  - minLat, minLon, maxLat, maxLon: Bounding box de la vue actuelle
 *  - zoom: Niveau de zoom Leaflet (0-18) pour calculer la taille de grille
 * 
 * Paramètres optionnels (query string):
 *  - status: Filtrer par statut (active, inactive, maintenance)
 *  - technology: Filtrer par technologie (2G, 3G, 4G, 5G)
 *  - operator_id: Filtrer par opérateur
 * 
 * Retour: GeoJSON FeatureCollection avec clusters
 *  - Si cluster: geometry = centroïde, properties contient count et liste des IDs
 *  - Si antenne unique: geometry = point exact, properties complètes
 */
void AntenneController::getClusteredAntennas(const HttpRequestPtr& req,
                                             std::function<void (const HttpResponsePtr &)> &&callback,
                                             double minLat, double minLon, double maxLat, double maxLon, int zoom) {
    // ========== VALIDATION DES PARAMÈTRES ==========
    Validator::ErrorCollector validator;
    
    // Validation bbox
    if (!Validator::isValidLatitude(minLat) || !Validator::isValidLatitude(maxLat)) {
        validator.addError("latitude", "Latitudes must be between -90 and +90 degrees");
    }
    if (!Validator::isValidLongitude(minLon) || !Validator::isValidLongitude(maxLon)) {
        validator.addError("longitude", "Longitudes must be between -180 and +180 degrees");
    }
    if (minLat >= maxLat) {
        validator.addError("bbox", "minLat must be less than maxLat");
    }
    if (minLon >= maxLon) {
        validator.addError("bbox", "minLon must be less than maxLon");
    }
    
    // Validation zoom (Leaflet: 0 = monde entier, 18 = max zoom)
    if (zoom < 0 || zoom > 18) {
        validator.addError("zoom", "Zoom level must be between 0 and 18");
    }
    
    if (validator.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validator.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }
    
    // ========== EXTRACTION DES FILTRES OPTIONNELS ==========
    std::string status = req->getOptionalParameter<std::string>("status").value_or("");
    std::string technology = req->getOptionalParameter<std::string>("technology").value_or("");
    int operator_id = req->getOptionalParameter<int>("operator_id").value_or(0);
    
    // Validation des filtres si fournis
    if (!status.empty() && !Validator::isValidStatus(status)) {
        auto resp = ErrorHandler::createGenericErrorResponse(
            "Invalid status. Must be one of: active, inactive, maintenance", 
            k400BadRequest
        );
        callback(resp);
        return;
    }
    
    if (!technology.empty() && !Validator::isValidTechnology(technology)) {
        auto resp = ErrorHandler::createGenericErrorResponse(
            "Invalid technology. Must be one of: 2G, 3G, 4G, 5G", 
            k400BadRequest
        );
        callback(resp);
        return;
    }
    
    // ========== SPRINT 3: CACHE REDIS ==========
    // Clé de cache basée sur bbox + zoom + filtres
    std::string cacheKey = "clusters:bbox:" + 
                          std::to_string(minLat) + ":" + std::to_string(minLon) + ":" + 
                          std::to_string(maxLat) + ":" + std::to_string(maxLon) + 
                          ":z:" + std::to_string(zoom);
    
    if (!status.empty()) cacheKey += ":st:" + status;
    if (!technology.empty()) cacheKey += ":tech:" + technology;
    if (operator_id > 0) cacheKey += ":op:" + std::to_string(operator_id);
    
    // Vérification cache
    auto cached = CacheService::getInstance().getCachedClusters(cacheKey);
    if (cached) {
        LOG_INFO << "✅ Cache HIT: " << cacheKey;
        
        auto resp = HttpResponse::newHttpJsonResponse(*cached);
        resp->addHeader("Content-Type", "application/geo+json");
        resp->addHeader("X-Cache", "HIT");
        resp->addHeader("Cache-Control", "public, max-age=120");
        callback(resp);
        return;
    }
    
    LOG_INFO << "❌ Cache MISS: " << cacheKey;
    
    // ========== VERROUILLAGE POUR ÉVITER LES CALCULS CONCURRENTS ==========
    std::string lockKey = "lock:" + cacheKey;
    bool hasLock = CacheService::getInstance().tryLock(lockKey, 60);
    
    if (!hasLock) {
        // Si lock pas acquis, attendre un peu et revérifier le cache
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto retryCached = CacheService::getInstance().getCachedClusters(cacheKey);
        if (retryCached) {
            LOG_INFO << "🔄 Cache HIT after lock retry: " << cacheKey;
            auto resp = HttpResponse::newHttpJsonResponse(*retryCached);
            resp->addHeader("Content-Type", "application/geo+json");
            resp->addHeader("X-Cache", "HIT-RETRY");
            resp->addHeader("Cache-Control", "public, max-age=120");
            callback(resp);
            return;
        }
        // Sinon, procéder sans lock pour éviter le blocage
        LOG_WARN << "⚠️ Proceeding without lock for: " << cacheKey;
    }
    
    // ========== APPEL AU SERVICE ==========
    AntenneService::getClusteredAntennas(
        minLat, minLon, maxLat, maxLon, zoom, status, technology, operator_id,
        [callback, zoom, minLat, minLon, maxLat, maxLon, cacheKey, hasLock, lockKey](const Json::Value& geojson, const std::string& err) {
            if (err.empty()) {
                // Ajout de métadonnées utiles pour le debug et le monitoring
                Json::Value response = geojson;
                response["metadata"]["zoom"] = zoom;
                response["metadata"]["bbox"] = Json::Value(Json::objectValue);
                response["metadata"]["bbox"]["minLat"] = minLat;
                response["metadata"]["bbox"]["minLon"] = minLon;
                response["metadata"]["bbox"]["maxLat"] = maxLat;
                response["metadata"]["bbox"]["maxLon"] = maxLon;
                
                // Sprint 3: Mise en cache (TTL 2min pour clusters)
                CacheService::getInstance().cacheClusters(cacheKey, response);
                LOG_INFO << "💾 Cached clusters: " << cacheKey;
                
                // Libérer le verrou
                if (hasLock) {
                    CacheService::getInstance().unlock(lockKey);
                }
                
                auto resp = HttpResponse::newHttpJsonResponse(response);
                resp->addHeader("Content-Type", "application/geo+json");
                resp->addHeader("X-Cache", "MISS");
                resp->addHeader("Cache-Control", "public, max-age=120");
                
                callback(resp);
            } else {
                // Libérer le verrou en cas d'erreur
                if (hasLock) {
                    CacheService::getInstance().unlock(lockKey);
                }
                
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::getClusteredAntennas", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        }
    );
}

// ============================================================================
// 10. GET SIMPLIFIED COVERAGE (Sprint 4 Performance - Navigation Fluide)
// ============================================================================
void AntenneController::getSimplifiedCoverage(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    double minLat, double minLon, double maxLat, double maxLon, int zoom)
{
    // ========== FILTRES OPTIONNELS ==========
    int operator_id = req->getOptionalParameter<int>("operator_id").value_or(0);
    std::string technology = req->getOptionalParameter<std::string>("technology").value_or("");
    
    LOG_INFO << "📡 GET /api/antennas/coverage/simplified - bbox: [" 
             << minLat << "," << minLon << " → " << maxLat << "," << maxLon 
             << "] zoom: " << zoom
             << " operator: " << operator_id
             << " tech: " << (technology.empty() ? "all" : technology);
    
    // ========== VALIDATION ==========
    if (minLat >= maxLat || minLon >= maxLon) {
        Json::Value error;
        error["error"] = "Invalid bounding box: minLat must be < maxLat and minLon must be < maxLon";
        error["details"]["minLat"] = minLat;
        error["details"]["minLon"] = minLon;
        error["details"]["maxLat"] = maxLat;
        error["details"]["maxLon"] = maxLon;
        
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    if (zoom < 0 || zoom > 18) {
        Json::Value error;
        error["error"] = "Zoom level must be between 0 and 18";
        error["details"]["zoom"] = zoom;
        
        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    
    // ========== SPRINT 3: CACHE REDIS ==========
    // Clé de cache : coverage simplifiée par bbox + zoom + filtres
    std::string cacheKey = "coverage:simplified:bbox:" + 
                          std::to_string(minLat) + ":" + std::to_string(minLon) + ":" + 
                          std::to_string(maxLat) + ":" + std::to_string(maxLon) + 
                          ":z:" + std::to_string(zoom) +
                          ":op:" + std::to_string(operator_id) +
                          ":tech:" + (technology.empty() ? "all" : technology);
    
    // Vérification cache (TTL 5min - stable car basé sur antennes actives)
    auto cached = CacheService::getInstance().getJson(cacheKey);
    if (cached) {
        LOG_INFO << "✅ Coverage Cache HIT: " << cacheKey;
        
        auto resp = HttpResponse::newHttpJsonResponse(*cached);
        resp->addHeader("Content-Type", "application/geo+json");
        resp->addHeader("X-Cache", "HIT");
        resp->addHeader("Cache-Control", "public, max-age=300"); // 5min
        callback(resp);
        return;
    }
    
    LOG_INFO << "❌ Coverage Cache MISS: " << cacheKey;
    
    // ========== APPEL AU SERVICE ==========
    AntenneService::getSimplifiedCoverage(
        minLat, minLon, maxLat, maxLon, zoom, operator_id, technology,
        [callback, cacheKey](const Json::Value& geojson, const std::string& err) {
            if (err.empty()) {
                // Sprint 3: Mise en cache (TTL 5min pour coverage)
                CacheService::getInstance().set(cacheKey, geojson.toStyledString(), 300);
                LOG_INFO << "💾 Cached coverage: " << cacheKey;
                
                auto resp = HttpResponse::newHttpJsonResponse(geojson);
                resp->addHeader("Content-Type", "application/geo+json");
                resp->addHeader("X-Cache", "MISS");
                resp->addHeader("Cache-Control", "public, max-age=300");
                
                callback(resp);
            } else {
                auto errorDetails = ErrorHandler::analyzePostgresError(err);
                ErrorHandler::logError("AntenneController::getSimplifiedCoverage", errorDetails);
                auto resp = ErrorHandler::createErrorResponse(errorDetails);
                callback(resp);
            }
        }
    );
}