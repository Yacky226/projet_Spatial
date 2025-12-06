#pragma once
#include <drogon/drogon.h>
#include <string>

// Structure représentant une zone géographique dans le système
// Utilisée pour la hiérarchie administrative et les zones de couverture
struct ZoneModel {
    int id = -1;                    // Identifiant unique (-1 si non défini)
    std::string name;               // Nom de la zone
    std::string type;               // Type de zone : country, region, province, coverage, white_zone
    double density = 0.0;           // Densité de population (habitants/km²)
    std::string wkt_geometry;       // Géométrie au format WKT pour PostGIS
    int parent_id = 0;              // ID de la zone parente (0 = racine)

    Json::Value toJson() const {
        Json::Value ret;
        if(id != -1) ret["id"] = id;
        ret["name"] = name;
        ret["type"] = type;
        ret["density"] = density;
        ret["wkt"] = wkt_geometry;
        if(parent_id > 0) ret["parent_id"] = parent_id;
        return ret;
    }
};