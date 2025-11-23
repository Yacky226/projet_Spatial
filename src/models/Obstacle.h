#pragma once
#include <drogon/drogon.h>
#include <string>

struct ObstacleModel {
    int id = -1;
    std::string type;      // Ex: 'batiment', 'foret', 'montagne'
    std::string geom_type; // 'POLYGON', 'LINESTRING', etc.
    std::string wkt_geometry; // Géométrie au format texte

    Json::Value toJson() const {
        Json::Value ret;
        if(id != -1) ret["id"] = id;
        ret["type"] = type;
        ret["geom_type"] = geom_type;
        ret["wkt"] = wkt_geometry;
        return ret;
    }
};