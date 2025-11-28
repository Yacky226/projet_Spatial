#pragma once
#include <drogon/drogon.h>
#include <json/json.h>
#include <string>
#include <vector>

struct ZoneCoverageDetail {
    std::string zone_name;
    double intersection_area_km2;
    double covered_population;
    
    Json::Value toJson() const {
        Json::Value ret;
        ret["zone_name"] = zone_name;
        ret["area_km2"] = intersection_area_km2;
        ret["population"] = covered_population;
        return ret;
    }
};

struct CoverageResult {
    int antenna_id;
    double total_population_covered = 0.0;
    double total_area_km2 = 0.0;
    std::vector<ZoneCoverageDetail> details;

    Json::Value toJson() const {
        Json::Value ret;
        ret["antenna_id"] = antenna_id;
        ret["total_population"] = total_population_covered;
        ret["total_area_km2"] = total_area_km2;
        
        Json::Value arr(Json::arrayValue);
        for (const auto& d : details) arr.append(d.toJson());
        ret["details"] = arr;
        
        return ret;
    }
};