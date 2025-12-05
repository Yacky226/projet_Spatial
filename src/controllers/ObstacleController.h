#pragma once
#include <drogon/HttpController.h>
#include "../services/ObstacleService.h"

using namespace drogon;

class ObstacleController : public drogon::HttpController<ObstacleController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ObstacleController::getByBoundingBox, "/api/obstacles/bbox", Get);
    METHOD_LIST_END

    void getByBoundingBox(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
};