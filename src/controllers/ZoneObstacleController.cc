#include "ZoneObstacleController.h"
#include "../services/ZoneObstacleService.h"
#include <json/json.h>

void ZoneObstacleController::getObstaclesByZone(const HttpRequestPtr &req,
                                                std::function<void(const HttpResponsePtr &)> &&callback,
                                                int zone_id) {
    ZoneObstacleService::getObstaclesByZone(zone_id,
                                            [callback](const std::vector<int> &obstacles, const std::string &err) {
                                                auto res = HttpResponse::newHttpResponse();
                                                if (err.empty()) {
                                                    Json::Value json(Json::arrayValue);
                                                    for (int obstacle_id : obstacles) {
                                                        json.append(obstacle_id);
                                                    }
                                                    res->setBody(Json::FastWriter().write(json));
                                                } else {
                                                    res->setStatusCode(k500InternalServerError);
                                                    res->setBody(err);
                                                }
                                                callback(res);
                                            });
}

void ZoneObstacleController::getZonesByObstacle(const HttpRequestPtr &req,
                                                std::function<void(const HttpResponsePtr &)> &&callback,
                                                int obstacle_id) {
    ZoneObstacleService::getZonesByObstacle(obstacle_id,
                                            [callback](const std::vector<int> &zones, const std::string &err) {
                                                auto res = HttpResponse::newHttpResponse();
                                                if (err.empty()) {
                                                    Json::Value json(Json::arrayValue);
                                                    for (int zone_id : zones) {
                                                        json.append(zone_id);
                                                    }
                                                    res->setBody(Json::FastWriter().write(json));
                                                } else {
                                                    res->setStatusCode(k500InternalServerError);
                                                    res->setBody(err);
                                                }
                                                callback(res);
                                            });
}