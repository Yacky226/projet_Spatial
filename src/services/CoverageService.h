#pragma once
#include <drogon/drogon.h>
#include <json/json.h>
#include "../models/CoverageResult.h"
#include <functional>
#include <string>

class CoverageService {
public:
    static void calculateForAntenna(int antennaId, 
                                    std::function<void(const CoverageResult&, const std::string&)> callback);
                                    
    static void getWhiteZones(int zone_id,
                              std::function<void(const Json::Value&, const std::string&)> callback);
};