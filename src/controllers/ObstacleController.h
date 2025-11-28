#pragma once
#include <drogon/HttpController.h>
#include "../services/ObstacleService.h"
#include "../utils/PaginationMeta.h"

using namespace drogon;

class ObstacleController : public drogon::HttpController<ObstacleController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ObstacleController::create, "/api/obstacles", Post);
        ADD_METHOD_TO(ObstacleController::getAll, "/api/obstacles", Get);
        ADD_METHOD_TO(ObstacleController::getById, "/api/obstacles/{1}", Get);
        ADD_METHOD_TO(ObstacleController::update, "/api/obstacles/{1}", Put);
        ADD_METHOD_TO(ObstacleController::remove, "/api/obstacles/{1}", Delete);  
        ADD_METHOD_TO(ObstacleController::getGeoJSON, "/api/obstacles/geojson", Get);
        ADD_METHOD_TO(ObstacleController::getByBoundingBox, "/api/obstacles/bbox", Get);
    METHOD_LIST_END

    void create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id); 
    void getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getByBoundingBox(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
};