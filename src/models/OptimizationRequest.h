#pragma once
#include <drogon/drogon.h>

struct OptimizationRequest {
    int zone_id;            // La zone à couvrir
    int operator_id;        // 🆕 L'opérateur qui veut optimiser son réseau
    int antennas_count;     // Nombre d'antennes à placer (ex: 3)
    double radius;          // Rayon de chaque antenne (ex: 500m)
    std::string technology; // "4G" ou "5G"

    static OptimizationRequest fromJson(const std::shared_ptr<Json::Value>& json) {
        OptimizationRequest req;
        req.zone_id = (*json).get("zone_id", 0).asInt();
        req.operator_id = (*json).get("operator_id", 0).asInt();  // 🆕
        req.antennas_count = (*json).get("antennas_count", 1).asInt();
        req.radius = (*json).get("radius", 1000.0).asDouble();
        req.technology = (*json).get("technology", "5G").asString();
        return req;
    }
};

struct OptimizationResult {
    double latitude;
    double longitude;
    double estimated_population;
    double uncovered_area_km2;     // 🆕 Surface non couverte par CET opérateur
    double coverage_improvement;    // 🆕 Amélioration de la couverture (%)
    int score;

    Json::Value toJson() const {
        Json::Value ret;
        ret["latitude"] = latitude;
        ret["longitude"] = longitude;
        ret["estimated_population"] = estimated_population;
        ret["uncovered_area_km2"] = uncovered_area_km2;
        ret["coverage_improvement"] = coverage_improvement;
        ret["score"] = score;
        return ret;
    }
};