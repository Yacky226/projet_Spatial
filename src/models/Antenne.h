#pragma once
#include <drogon/drogon.h>
#include <string>

// Représentation C++ de la table 'antenna'
struct Antenna {
    int id = -1;
    double coverage_radius;
    std::string status;       // Enum 'antenna_status' converti en string
    std::string technology;   // Enum 'technology_type' (2G, 4G, 5G)
    std::string installation_date;
    int operator_id;

    // Pour l'API : on sépare la géométrie en lat/lon
    double latitude;
    double longitude;

    // Conversion en JSON pour la réponse API
    Json::Value toJson() const {
        Json::Value ret;
        if(id != -1) ret["id"] = id;
        ret["coverage_radius"] = coverage_radius;
        ret["status"] = status;
        ret["technology"] = technology;
        ret["operator_id"] = operator_id;
        ret["latitude"] = latitude;
        ret["longitude"] = longitude;
        return ret;
    }
};