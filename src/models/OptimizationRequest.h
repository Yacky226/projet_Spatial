#pragma once
#include <drogon/drogon.h>
#include <optional>

struct OptimizationRequest {
    std::optional<int> zone_id;           // La zone à couvrir (optionnel si bbox fourni)
    std::optional<std::string> bbox_wkt;  // BBox en WKT (optionnel si zone_id fourni)
    int antennas_count;                   // Nombre d'antennes à placer (ex: 3)
    double radius;                        // Rayon de chaque antenne (ex: 500m)
    std::string technology;               // "4G" ou "5G"

    static OptimizationRequest fromJson(const std::shared_ptr<Json::Value>& json) {
        OptimizationRequest req;
        
        // Zone ID (optionnel)
        if ((*json).isMember("zone_id") && !(*json)["zone_id"].isNull()) {
            req.zone_id = (*json)["zone_id"].asInt();
        }
        
        // Bounding box WKT (optionnel)
        if ((*json).isMember("bbox_wkt") && !(*json)["bbox_wkt"].isNull()) {
            req.bbox_wkt = (*json)["bbox_wkt"].asString();
        }
        
        req.antennas_count = (*json).get("antennas_count", 1).asInt();
        req.radius = (*json).get("radius", 1000.0).asDouble();
        req.technology = (*json).get("technology", "5G").asString();
        return req;
    }
    
    // Validation: zone_id XOR bbox_wkt
    bool isValid() const {
        return (zone_id.has_value() && !bbox_wkt.has_value()) ||
               (!zone_id.has_value() && bbox_wkt.has_value());
    }
    
    // Helper pour savoir quel mode on utilise
    bool isZoneMode() const {
        return zone_id.has_value();
    }
};

struct OptimizationResult {
    double latitude;
    double longitude;
    double estimated_population;
    int score; // Un score arbitraire (ex: 0-100)

    Json::Value toJson() const {
        Json::Value ret;
        ret["latitude"] = latitude;
        ret["longitude"] = longitude;
        ret["estimated_population"] = estimated_population;
        ret["score"] = score;
        return ret;
    }
};