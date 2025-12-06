#pragma once
#include <drogon/drogon.h>
#include <string>

// Structure pour représenter une antenne dans l'API
// Correspond à la table 'antenna' en base de données
struct Antenna {
    int id = -1;
    double coverage_radius;
    std::string status;       // Enum 'antenna_status' converti en string
    std::string technology;   // Enum 'technology_type' (2G, 4G, 5G)
    std::string installation_date;
    int operator_id;
    std::string operatorName; // Nom de l'opérateur (jointure)

    // Pour l'API : on sépare la géométrie en lat/lon
    double latitude;
    double longitude;

    // Sérialisation en JSON pour l'API
    Json::Value toJson() const {
        Json::Value ret;
        if(id != -1) ret["id"] = id;
        ret["coverage_radius"] = coverage_radius;
        ret["status"] = status;
        ret["technology"] = technology;
        ret["operator_id"] = operator_id;
        ret["operatorName"] = operatorName;
        ret["latitude"] = latitude;
        ret["longitude"] = longitude;
        return ret;
    }
};