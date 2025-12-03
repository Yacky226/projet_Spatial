#pragma once
#include "../models/Zone.h"
#include <drogon/drogon.h>
#include <vector>
#include <string>
#include <functional>

class ZoneService {
public:
    static void create(const ZoneModel &z, std::function<void(const std::string&)> callback);
    static void getAll(std::function<void(const std::vector<ZoneModel>&, const std::string&)> callback);
    static void getById(int id, std::function<void(const ZoneModel&, const std::string&)> callback);
    static void getByType(const std::string& type, std::function<void(const std::vector<ZoneModel>&, const std::string&)> callback);
    
    // Sprint 2: Simplification géométrique selon le zoom
    static void getByTypeSimplified(const std::string& type, int zoom, 
                                    std::function<void(const std::vector<ZoneModel>&, const std::string&)> callback);
    
    static void update(const ZoneModel &z, std::function<void(const std::string&)> callback);
    static void remove(int id, std::function<void(const std::string&)> callback);
    static void getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback);
    static void getWhiteZones(int zone_id, int operator_id,std::function<void(const Json::Value&, const std::string&)> callback);
    static void searchZones(const std::string& type, const std::string& query, int limit, 
                           std::function<void(const std::vector<ZoneModel>&, const std::string&)> callback);

private:
    // Utilitaire pour calculer la tolérance de simplification selon le zoom
    static double calculateSimplificationTolerance(int zoom);
};