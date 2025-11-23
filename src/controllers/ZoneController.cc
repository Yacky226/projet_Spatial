#include "ZoneController.h"

// 1. Create
void ZoneController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) { /* 400 Bad Request */ return; }

    ZoneModel z;
    z.name = (*json).get("name", "").asString();
    z.type = (*json).get("type", "coverage").asString();
    z.density = (*json).get("density", 0.0).asDouble();
    z.wkt_geometry = (*json).get("wkt", "").asString(); // Ex: "POLYGON((...))"
    z.parent_id = (*json).get("parent_id", 0).asInt();

    ZoneService::create(z, [callback](const std::string& err){
        if(err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k201Created);
            resp->setBody("Zone created");
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
void ZoneController::getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    ZoneService::getAll([callback](const std::vector<ZoneModel>& list, const std::string& err) {
        if(err.empty()){
            Json::Value arr(Json::arrayValue);
            for(auto &z : list) arr.append(z.toJson());
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            callback(resp);
        } else {
            /* 500 Error */
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 3. Read One
void ZoneController::getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    ZoneService::getById(id, [callback](const ZoneModel& z, const std::string& err){
        if(err.empty()){
            auto resp = HttpResponse::newHttpJsonResponse(z.toJson());
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            callback(resp);
        }
    });
}

// 4. Update
void ZoneController::update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    auto json = req->getJsonObject();
    if(!json) { /* 400 */ return; }

    ZoneModel z;
    z.id = id;
    z.name = (*json).get("name", "").asString();
    z.type = (*json).get("type", "coverage").asString();
    z.density = (*json).get("density", 0.0).asDouble();
    z.wkt_geometry = (*json).get("wkt", "").asString();
    z.parent_id = (*json).get("parent_id", 0).asInt();

    ZoneService::update(z, [callback](const std::string& err){
        if(err.empty()){
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setBody("Zone updated");
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
void ZoneController::remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    ZoneService::remove(id, [callback](const std::string& err){
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

void ZoneController::getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    ZoneService::getAllGeoJSON([callback](const Json::Value& json, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(json);
            callback(resp); // Renvoie la FeatureCollection complète
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}