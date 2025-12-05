#include "CacheService.h"
#include <drogon/drogon.h>
#include <thread>
#include <chrono>

using namespace drogon;

CacheService& CacheService::getInstance() {
    static CacheService instance;
    return instance;
}

void CacheService::init(const std::string& host, int port, const std::string& password) {
    int maxRetries = 5;
    int retryDelay = 1000; // ms
    
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        try {
            ConnectionOptions opts;
            opts.host = host;
            opts.port = port;
            if (!password.empty()) {
                opts.password = password;
            }
            opts.socket_timeout = std::chrono::milliseconds(1000);  // Augmenté à 1s
            opts.connect_timeout = std::chrono::milliseconds(2000); // Ajout timeout connexion
            opts.keep_alive = true;
            
            redis_ = std::make_unique<Redis>(opts);
            
            // Test connexion
            redis_->ping();
            LOG_INFO << "✅ Redis connected: " << host << ":" << port;
            return;
        } catch (const Error& e) {
            LOG_WARN << "❌ Redis connection attempt " << attempt << "/" << maxRetries << " failed: " << e.what();
            redis_.reset();
            if (attempt < maxRetries) {
                std::this_thread::sleep_for(std::chrono::milliseconds(retryDelay));
            }
        }
    }
    throw std::runtime_error("Failed to connect to Redis after " + std::to_string(maxRetries) + " attempts");
}

void CacheService::set(const std::string& key, const std::string& value, int ttl_seconds) {
    if (!redis_) return;
    try {
        redis_->set(key, value);
        if (ttl_seconds > 0) {
            redis_->expire(key, ttl_seconds);
        }
    } catch (const Error& e) {
        LOG_WARN << "Redis SET error for key '" << key << "': " << e.what();
    }
}

std::optional<std::string> CacheService::get(const std::string& key) {
    if (!redis_) return std::nullopt;
    try {
        auto val = redis_->get(key);
        if (val) {
            return *val;
        }
    } catch (const Error& e) {
        LOG_WARN << "Redis GET error for key '" << key << "': " << e.what();
    }
    return std::nullopt;
}

void CacheService::setJson(const std::string& key, const Json::Value& data, int ttl_seconds) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string jsonStr = Json::writeString(builder, data);
    set(key, jsonStr, ttl_seconds);
}

std::optional<Json::Value> CacheService::getJson(const std::string& key) {
    auto cached = get(key);
    if (!cached) return std::nullopt;
    
    Json::Value result;
    Json::CharReaderBuilder builder;
    std::string errors;
    std::istringstream stream(*cached);
    
    if (Json::parseFromStream(builder, stream, &result, &errors)) {
        return result;
    }
    
    LOG_WARN << "Failed to parse cached JSON for key '" << key << "': " << errors;
    return std::nullopt;
}

void CacheService::del(const std::string& key) {
    if (!redis_) return;
    try {
        redis_->del(key);
    } catch (const Error& e) {
        LOG_WARN << "Redis DEL error for key '" << key << "': " << e.what();
    }
}

void CacheService::delPattern(const std::string& pattern) {
    if (!redis_) return;
    try {
        std::vector<std::string> keys;
        redis_->keys(pattern, std::back_inserter(keys));
        
        if (!keys.empty()) {
            redis_->del(keys.begin(), keys.end());
            LOG_INFO << "🗑️ Deleted " << keys.size() << " keys matching: " << pattern;
        }
    } catch (const Error& e) {
        LOG_WARN << "Redis DEL pattern error for '" << pattern << "': " << e.what();
    }
}

bool CacheService::tryLock(const std::string& key, int ttl_seconds) {
    if (!redis_) return false;
    try {
        auto result = redis_->set(key, "locked", std::chrono::seconds(ttl_seconds), sw::redis::UpdateType::NOT_EXIST);
        return result;
    } catch (const Error& e) {
        LOG_WARN << "Redis lock error for key '" << key << "': " << e.what();
        return false;
    }
}

void CacheService::unlock(const std::string& key) {
    del(key);
}
