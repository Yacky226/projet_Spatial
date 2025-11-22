#pragma once
#include "../models/Antenne.h"
#include <drogon/drogon.h>
#include <functional> // Ajout de sécurité
#include <vector>
#include <string>

class AntenneService {
public:
    static void createAntenne(const AntenneModel &antenne, 
                              std::function<void(const std::string& error)> callback);

    static void getAllAntennes(std::function<void(const std::vector<AntenneModel>&, const std::string&)> callback);
};