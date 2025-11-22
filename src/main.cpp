#include <drogon/drogon.h>
#include <iostream>

int main() {
    // 1. Désactiver le buffering pour que Docker voie les logs tout de suite
    setbuf(stdout, NULL);
    std::cout << ">>> Démarrage de l'application..." << std::endl;

    // 2. Charger la config
    drogon::app().loadConfigFile("../config/config.json");
    
    // 3. Lancer le serveur
    drogon::app().run();
    return 0;
}