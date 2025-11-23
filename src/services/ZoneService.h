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
    static void update(const ZoneModel &z, std::function<void(const std::string&)> callback);
    static void remove(int id, std::function<void(const std::string&)> callback);
    static void getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback);

};