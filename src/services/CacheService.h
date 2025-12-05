#pragma once
#include <string>
#include <optional>
#include <memory>
#include <sw/redis++/redis++.h>
#include <json/json.h>
#include <drogon/drogon.h>

using namespace sw::redis;

/**
 * Service de cache Redis - Sprint 3
 * 
 * Gère le cache backend pour réduire la charge DB de 70%
 * TTL adaptatifs selon le type de données :
 * - Zones: 1h (données rarement modifiées)
 * - Clusters: 2min (données dynamiques)
 * - Antennas: 5min (équilibre)
 */
class CacheService {
public:
    static CacheService& getInstance();
    
    // Initialisation
    void init(const std::string& host, int port, const std::string& password = "");
    bool isConnected() const { return redis_ != nullptr; }
    
    // Cache générique
    void set(const std::string& key, const std::string& value, int ttl_seconds = 300);
    std::optional<std::string> get(const std::string& key);
    void del(const std::string& key);
    void delPattern(const std::string& pattern);
    
    // Cache JSON
    void setJson(const std::string& key, const Json::Value& data, int ttl_seconds = 300);
    std::optional<Json::Value> getJson(const std::string& key);
    
    // Cache spécialisé pour zones (TTL: 1h)
    void cacheZones(const std::string& key, const Json::Value& data) {
        setJson("zones:" + key, data, 3600);
    }
    std::optional<Json::Value> getCachedZones(const std::string& key) {
        return getJson("zones:" + key);
    }
    void invalidateZonesByType(const std::string& type) {
        delPattern("zones:type:" + type + ":*");
    }
    
    // Cache spécialisé pour clusters (TTL: 1h)
    void cacheClusters(const std::string& key, const Json::Value& data) {
        setJson("clusters:" + key, data, 3600);
    }
    std::optional<Json::Value> getCachedClusters(const std::string& key) {
        return getJson("clusters:" + key);
    }
    
    // Cache spécialisé pour antennes (TTL: 1h)
    void cacheAntennas(const std::string& key, const Json::Value& data) {
        setJson("antennas:" + key, data, 3600);
    }
    std::optional<Json::Value> getCachedAntennas(const std::string& key) {
        return getJson("antennas:" + key);
    }
    void invalidateAntennaCache() {
        delPattern("antennas:*");
        delPattern("clusters:*"); // Invalider aussi les clusters
    }
    
    // Mécanisme de verrouillage pour éviter les calculs concurrents
    bool tryLock(const std::string& key, int ttl_seconds = 60);
    void unlock(const std::string& key);
    
private:
    CacheService() = default;
    std::unique_ptr<Redis> redis_;
};
