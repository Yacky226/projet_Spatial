#pragma once
#include <drogon/HttpController.h>
#include "../services/OperatorService.h"

using namespace drogon;

class OperatorController : public drogon::HttpController<OperatorController> {
public:
    METHOD_LIST_BEGIN
        // Read All
        ADD_METHOD_TO(OperatorController::getAll, "/api/operators", Get);
    METHOD_LIST_END

    void getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
};