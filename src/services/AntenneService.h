#pragma once
#include "../models/Antenne.h"
#include <drogon/drogon.h>
#include <vector>
#include <string>
#include <functional>

class AntenneService {
public:
    static void create(const Antenna &antenne, std::function<void(const std::string&)> callback);
    static void getAll(std::function<void(const std::vector<Antenna>&, const std::string&)> callback);
    static void getById(int id, std::function<void(const Antenna&, const std::string&)> callback);
    static void update(const Antenna &antenne, std::function<void(const std::string&)> callback);
    static void remove(int id, std::function<void(const std::string&)> callback);

    // Ajoutez ceci dans la classe AntenneService (en public)
static void getInRadius(double lat, double lon, double radiusMeters, 
                        std::function<void(const std::vector<Antenna>&, const std::string&)> callback);

static void getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback);
};