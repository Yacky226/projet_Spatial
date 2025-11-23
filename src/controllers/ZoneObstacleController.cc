#include "ZoneObstacleController.h"
#include "../services/ZoneObstacleService.h"
#include <json/json.h>

void ZoneObstacleController::linkZoneObstacle(const HttpRequestPtr &req,
                                              std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("zone_id") || !json->isMember("obstacle_id")) {
        auto res = HttpResponse::newHttpResponse();
        res->setStatusCode(k400BadRequest);
        res->setBody("Missing zone_id or obstacle_id");
        callback(res);
        return;
    }

    int zone_id = (*json)["zone_id"].asInt();
    int obstacle_id = (*json)["obstacle_id"].asInt();

    ZoneObstacleService::link(zone_id, obstacle_id, [callback](const std::string &err) {
        auto res = HttpResponse::newHttpResponse();
        if (err.empty()) {
            res->setStatusCode(k200OK);
            res->setBody("Link created");
        } else {
            res->setStatusCode(k500InternalServerError);
            res->setBody(err);
        }
        callback(res);
    });
}

void ZoneObstacleController::unlinkZoneObstacle(const HttpRequestPtr &req,
                                                std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("zone_id") || !json->isMember("obstacle_id")) {
        auto res = HttpResponse::newHttpResponse();
        res->setStatusCode(k400BadRequest);
        res->setBody("Missing zone_id or obstacle_id");
        callback(res);
        return;
    }

    int zone_id = (*json)["zone_id"].asInt();
    int obstacle_id = (*json)["obstacle_id"].asInt();

    ZoneObstacleService::unlink(zone_id, obstacle_id, [callback](const std::string &err) {
        auto res = HttpResponse::newHttpResponse();
        if (err.empty()) {
            res->setStatusCode(k200OK);
            res->setBody("Link removed");
        } else {
            res->setStatusCode(k500InternalServerError);
            res->setBody(err);
        }
        callback(res);
    });
}

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

void ZoneObstacleController::getAllLinks(const HttpRequestPtr &req,
                                         std::function<void(const HttpResponsePtr &)> &&callback) {
    ZoneObstacleService::getAll([callback](const std::vector<ZoneObstacleLink> &links, const std::string &err) {
        auto res = HttpResponse::newHttpResponse();
        if (err.empty()) {
            Json::Value json(Json::arrayValue);
            for (const auto &link : links) {
                Json::Value obj;
                obj["zone_id"] = link.zone_id;
                obj["obstacle_id"] = link.obstacle_id;
                json.append(obj);
            }
            res->setBody(Json::FastWriter().write(json));
        } else {
            res->setStatusCode(k500InternalServerError);
            res->setBody(err);
        }
        callback(res);
    });
}