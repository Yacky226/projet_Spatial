#pragma once
#include <drogon/HttpController.h>
#include "../services/ZoneService.h"
using namespace drogon;

class ZoneController : public drogon::HttpController<ZoneController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ZoneController::create, "/api/zones", Post);
        ADD_METHOD_TO(ZoneController::getAll, "/api/zones", Get);
        ADD_METHOD_TO(ZoneController::getById, "/api/zones/{1}", Get);
        ADD_METHOD_TO(ZoneController::update, "/api/zones/{1}", Put);
        ADD_METHOD_TO(ZoneController::remove, "/api/zones/{1}", Delete);
        ADD_METHOD_TO(ZoneController::getGeoJSON, "/api/zones/geojson", Get);
    METHOD_LIST_END

    void create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
};