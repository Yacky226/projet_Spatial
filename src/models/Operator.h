#pragma once
#include <drogon/drogon.h>
#include <string>

// Structure représentant un opérateur de télécommunications
struct OperatorModel {
    int id = -1;           // Identifiant unique de l'opérateur (-1 si non défini)
    std::string name;      // Nom de l'opérateur

    Json::Value toJson() const {
        Json::Value ret;
        if (id != -1) ret["id"] = id;
        ret["name"] = name;
        return ret;
    }
};