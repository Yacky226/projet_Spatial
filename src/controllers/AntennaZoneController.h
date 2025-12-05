#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class AntennaZoneController : public drogon::HttpController<AntennaZoneController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AntennaZoneController::getAntennasByZone, "/zone/{zone_id}/antennas", Get);
    METHOD_LIST_END

    void getAntennasByZone(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int zone_id);
};