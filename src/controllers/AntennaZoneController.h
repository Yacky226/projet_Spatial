#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class AntennaZoneController : public drogon::HttpController<AntennaZoneController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AntennaZoneController::linkAntennaZone, "/antenna-zone/link", Post);
    ADD_METHOD_TO(AntennaZoneController::unlinkAntennaZone, "/antenna-zone/unlink", Post);
    ADD_METHOD_TO(AntennaZoneController::getZonesByAntenna, "/antenna/{antenna_id}/zones", Get);
    ADD_METHOD_TO(AntennaZoneController::getAntennasByZone, "/zone/{zone_id}/antennas", Get);
    ADD_METHOD_TO(AntennaZoneController::getAllLinks, "/antenna-zone/all", Get);
    METHOD_LIST_END

    void linkAntennaZone(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void unlinkAntennaZone(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getZonesByAntenna(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int antenna_id);
    void getAntennasByZone(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int zone_id);
    void getAllLinks(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};