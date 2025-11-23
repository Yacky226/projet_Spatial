#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ZoneObstacleController : public drogon::HttpController<ZoneObstacleController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ZoneObstacleController::linkZoneObstacle, "/zone-obstacle/link", Post);
    ADD_METHOD_TO(ZoneObstacleController::unlinkZoneObstacle, "/zone-obstacle/unlink", Post);
    ADD_METHOD_TO(ZoneObstacleController::getObstaclesByZone, "/zone/{zone_id}/obstacles", Get);
    ADD_METHOD_TO(ZoneObstacleController::getZonesByObstacle, "/obstacle/{obstacle_id}/zones", Get);
    ADD_METHOD_TO(ZoneObstacleController::getAllLinks, "/zone-obstacle/all", Get);
    METHOD_LIST_END

    void linkZoneObstacle(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void unlinkZoneObstacle(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getObstaclesByZone(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int zone_id);
    void getZonesByObstacle(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int obstacle_id);
    void getAllLinks(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};