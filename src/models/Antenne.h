#pragma once
#include <drogon/drogon.h>
#include <string>

struct AntenneModel {
    int id = -1;
    std::string technology;
    double coverage_radius;
    int operator_id;
    double latitude;
    double longitude;

    Json::Value toJson() const {
        Json::Value ret;
        if(id != -1) ret["id"] = id;
        ret["technology"] = technology;
        ret["coverage_radius"] = coverage_radius;
        ret["operator_id"] = operator_id;
        ret["latitude"] = latitude;
        ret["longitude"] = longitude;
        return ret;
    }
};