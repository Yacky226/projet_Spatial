#pragma once
#include <drogon/HttpController.h>
#include "../services/SimulationService.h"

using namespace drogon;

class SimulationController : public drogon::HttpController<SimulationController> {
public:
    METHOD_LIST_BEGIN
        // Endpoint : /api/simulation/check?lat=...&lon=...&operatorId=...&technology=...
        ADD_METHOD_TO(SimulationController::checkSignal, "/api/simulation/check", Get);
    METHOD_LIST_END

    void checkSignal(const HttpRequestPtr& req,
                     std::function<void (const HttpResponsePtr &)> &&callback);
};