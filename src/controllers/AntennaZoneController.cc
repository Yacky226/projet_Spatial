#include "AntennaZoneController.h"
#include "../services/AntennaZoneService.h"
#include <json/json.h>

void AntennaZoneController::linkAntennaZone(const HttpRequestPtr &req,
                                             std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("antenna_id") || !json->isMember("zone_id")) {
        auto res = HttpResponse::newHttpResponse();
        res->setStatusCode(k400BadRequest);
        res->setBody("Missing antenna_id or zone_id");
        callback(res);
        return;
    }

    int antenna_id = (*json)["antenna_id"].asInt();
    int zone_id = (*json)["zone_id"].asInt();

    // On capture callback par copie, ce qui est valide même si c'est une rvalue ref à l'origine
    AntennaZoneService::link(antenna_id, zone_id, [callback](const std::string &err) {
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

void AntennaZoneController::unlinkAntennaZone(const HttpRequestPtr &req,
                                               std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("antenna_id") || !json->isMember("zone_id")) {
        auto res = HttpResponse::newHttpResponse();
        res->setStatusCode(k400BadRequest);
        res->setBody("Missing antenna_id or zone_id");
        callback(res);
        return;
    }

    int antenna_id = (*json)["antenna_id"].asInt();
    int zone_id = (*json)["zone_id"].asInt();

    AntennaZoneService::unlink(antenna_id, zone_id, [callback](const std::string &err) {
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

void AntennaZoneController::getZonesByAntenna(const HttpRequestPtr &req,
                                               std::function<void(const HttpResponsePtr &)> &&callback,
                                               int antenna_id) {
    AntennaZoneService::getZonesByAntenna(antenna_id,
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

void AntennaZoneController::getAntennasByZone(const HttpRequestPtr &req,
                                               std::function<void(const HttpResponsePtr &)> &&callback,
                                               int zone_id) {
    AntennaZoneService::getAntennasByZone(zone_id,
                                          [callback](const std::vector<int> &antennas, const std::string &err) {
                                              auto res = HttpResponse::newHttpResponse();
                                              if (err.empty()) {
                                                  Json::Value json(Json::arrayValue);
                                                  for (int antenna_id : antennas) {
                                                      json.append(antenna_id);
                                                  }
                                                  res->setBody(Json::FastWriter().write(json));
                                              } else {
                                                  res->setStatusCode(k500InternalServerError);
                                                  res->setBody(err);
                                              }
                                              callback(res);
                                          });
}

void AntennaZoneController::getAllLinks(const HttpRequestPtr &req,
                                         std::function<void(const HttpResponsePtr &)> &&callback) {
    AntennaZoneService::getAll([callback](const std::vector<AntennaZoneLink> &links, const std::string &err) {
        auto res = HttpResponse::newHttpResponse();
        if (err.empty()) {
            Json::Value json(Json::arrayValue);
            for (const auto &link : links) {
                Json::Value obj;
                obj["antenna_id"] = link.antenna_id;
                obj["zone_id"] = link.zone_id;
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