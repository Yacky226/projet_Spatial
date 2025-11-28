#pragma once
#include <drogon/HttpController.h>
#include "../services/SimulationService.h"

using namespace drogon;

class SimulationController : public drogon::HttpController<SimulationController> {
public:
    METHOD_LIST_BEGIN
        // Endpoint : /api/simulation/check?lat=...&lon=...
        ADD_METHOD_TO(SimulationController::checkSignal, "/api/simulation/check?lat={1}&lon={2}", Get);
    METHOD_LIST_END

    void checkSignal(const HttpRequestPtr& req, 
                     std::function<void (const HttpResponsePtr &)> &&callback,
                     double lat, double lon);
};