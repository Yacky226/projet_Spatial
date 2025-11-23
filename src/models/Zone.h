#pragma once
#include <drogon/drogon.h>
#include <string>

struct ZoneModel {
    int id = -1;
    std::string name;
    std::string type; // 'country', 'region', 'province', 'coverage', 'white_zone'
    double density = 0.0;
    std::string wkt_geometry; // Format texte WKT pour PostGIS
    int parent_id = 0; // 0 signifie "pas de parent"

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