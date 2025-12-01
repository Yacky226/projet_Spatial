#include "CacheService.h"
#include <drogon/drogon.h>

using namespace drogon;

CacheService& CacheService::getInstance() {
    static CacheService instance;
    return instance;
}

void CacheService::init(const std::string& host, int port, const std::string& password) {
    try {
        ConnectionOptions opts;
        opts.host = host;
        opts.port = port;
        if (!password.empty()) {
            opts.password = password;
        }
        opts.socket_timeout = std::chrono::milliseconds(100);
        opts.keep_alive = true;
        
        redis_ = std::make_unique<Redis>(opts);
        
        // Test connexion
        redis_->ping();
        LOG_INFO << "✅ Redis connected: " << host << ":" << port;
    } catch (const Error& e) {
        LOG_ERROR << "❌ Redis connection failed: " << e.what();
        redis_.reset();
        throw;
    }
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
