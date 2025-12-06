#include <drogon/drogon.h>
#include <iostream>
#include "services/CacheService.h"

int main() {
    // Pas de buffering pour voir les logs tout de suite
    setbuf(stdout, NULL);
    std::cout << ">>> Démarrage de l'application..." << std::endl;

    // Chargement de la config
    drogon::app().loadConfigFile("../config/config.json");

    // Setup Redis
    std::string redisHost = std::getenv("REDIS_HOST") ? std::getenv("REDIS_HOST") : "localhost";
    int redisPort = std::getenv("REDIS_PORT") ? std::stoi(std::getenv("REDIS_PORT")) : 6379;
    std::string redisPass = std::getenv("REDIS_PASSWORD") ? std::getenv("REDIS_PASSWORD") : "";

    try {
        CacheService::getInstance().init(redisHost, redisPort, redisPass);
        LOG_INFO << "🚀 Redis cache enabled (TTL: zones 1h, clusters 1h, antennas 5min)";
    } catch (const std::exception& e) {
        LOG_WARN << "⚠️ Redis unavailable, running without cache: " << e.what();
    }

    // Headers CORS pour éviter les problèmes de cross-origin
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &, const drogon::HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    });

    // Démarrer le serveur web Drogon
    drogon::app().run();
    return 0;
}