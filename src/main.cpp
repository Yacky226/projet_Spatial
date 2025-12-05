#include <drogon/drogon.h>
#include <iostream>
#include "services/CacheService.h"

int main() {
    setbuf(stdout, NULL);
    std::cout << ">>> Démarrage de l'application..." << std::endl;

    drogon::app().loadConfigFile("../config/config.json");
    
    // Initialisation du service de cache Redis
    std::string redisHost = std::getenv("REDIS_HOST") ? std::getenv("REDIS_HOST") : "localhost";
    int redisPort = std::getenv("REDIS_PORT") ? std::stoi(std::getenv("REDIS_PORT")) : 6379;
    std::string redisPass = std::getenv("REDIS_PASSWORD") ? std::getenv("REDIS_PASSWORD") : "";
    
    try {
        CacheService::getInstance().init(redisHost, redisPort, redisPass);
        LOG_INFO << "🚀 Redis cache enabled (TTL: zones 1h, clusters 2min, antennas 5min)";
    } catch (const std::exception& e) {
        LOG_WARN << "⚠️ Redis unavailable, running without cache: " << e.what();
    }

    // Ajout global des headers CORS pour les réponses standards
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &, const drogon::HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    });
    
    drogon::app().run();
    return 0;
}