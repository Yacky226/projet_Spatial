#pragma once
#include <drogon/HttpController.h>
#include "../services/CoverageService.h"

using namespace drogon;

class CoverageController : public drogon::HttpController<CoverageController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(CoverageController::getAntennaCoverage, "/api/coverage/antenna/{1}", Get);
        ADD_METHOD_TO(CoverageController::getWhiteZones, "/api/coverage/white-zones/{1}", Get);
        ADD_METHOD_TO(CoverageController::getGlobalStats, "/api/coverage/stats", Get);
    METHOD_LIST_END

    void getAntennaCoverage(const HttpRequestPtr& req, 
                            std::function<void (const HttpResponsePtr &)> &&callback, 
                            int antennaId);
                            
    void getWhiteZones(const HttpRequestPtr& req, 
                       std::function<void(const HttpResponsePtr&)>&& callback, 
                       int zone_id);
                       
    void getGlobalStats(const HttpRequestPtr& req, 
                        std::function<void(const HttpResponsePtr&)>&& callback);
};