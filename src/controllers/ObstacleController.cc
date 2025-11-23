#include "ObstacleController.h"

// 1. Create
void ObstacleController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) { /* 400 */ return; }

    ObstacleModel o;
    o.type = (*json).get("type", "batiment").asString();
    o.geom_type = (*json).get("geom_type", "POLYGON").asString();
    o.wkt_geometry = (*json).get("wkt", "").asString();

    ObstacleService::create(o, [callback](const std::string& err){
        if(err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k201Created);
            resp->setBody("Obstacle created");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 2. Read All
void ObstacleController::getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    ObstacleService::getAll([callback](const std::vector<ObstacleModel>& list, const std::string& err) {
        if(err.empty()){
            Json::Value arr(Json::arrayValue);
            for(auto &item : list) arr.append(item.toJson());
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            callback(resp);
        } else {
            /* 500 */
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 3. Read One
void ObstacleController::getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    ObstacleService::getById(id, [callback](const ObstacleModel& o, const std::string& err){
        if(err.empty()){
            auto resp = HttpResponse::newHttpJsonResponse(o.toJson());
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            callback(resp);
        }
    });
}

// 4. Update
void ObstacleController::update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    auto json = req->getJsonObject();
    if(!json) { /* 400 */ return; }

    ObstacleModel o;
    o.id = id;
    o.type = (*json).get("type", "batiment").asString();
    o.geom_type = (*json).get("geom_type", "POLYGON").asString();
    o.wkt_geometry = (*json).get("wkt", "").asString();

    ObstacleService::update(o, [callback](const std::string& err){
        if(err.empty()){
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setBody("Obstacle updated");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 5. Delete
void ObstacleController::remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    ObstacleService::remove(id, [callback](const std::string& err){
        if(err.empty()){
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k204NoContent);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

void ObstacleController::getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    ObstacleService::getAllGeoJSON([callback](const Json::Value& json, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(json);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}