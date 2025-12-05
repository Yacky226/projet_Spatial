#include "AntennaZoneController.h"
#include "../services/AntennaZoneService.h"
#include <json/json.h>

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