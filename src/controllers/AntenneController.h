#pragma once
#include <drogon/HttpController.h>
#include "../services/AntenneService.h"

using namespace drogon;

class AntenneController : public drogon::HttpController<AntenneController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AntenneController::create, "/api/antennes", Post);
        ADD_METHOD_TO(AntenneController::getAll, "/api/antennes", Get);
        ADD_METHOD_TO(AntenneController::getById, "/api/antennes/{1}", Get);
        ADD_METHOD_TO(AntenneController::update, "/api/antennes/{1}", Put);
        ADD_METHOD_TO(AntenneController::remove, "/api/antennes/{1}", Delete);
        ADD_METHOD_TO(AntenneController::search, "/api/antennes/search?lat={1}&lon={2}&radius={3}", Get);
        ADD_METHOD_TO(AntenneController::getGeoJSON, "/api/antennes/geojson", Get);

    METHOD_LIST_END

    void create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);

    void search(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, 
                double lat, double lon, double radius);
    void getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
};