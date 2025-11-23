#include <drogon/drogon.h>
#include <iostream>

int main() {
    setbuf(stdout, NULL);
    std::cout << ">>> Démarrage de l'application..." << std::endl;

    drogon::app().loadConfigFile("../config/config.json");

    // Ajout global des headers CORS pour les réponses standards
    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &, const drogon::HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    });
    
    drogon::app().run();
    return 0;
}