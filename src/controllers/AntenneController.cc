#include "AntenneController.h"

void AntenneController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    AntenneModel a;
    a.technology = (*json).get("technology", "4G").asString();
    a.coverage_radius = (*json).get("coverage_radius", 1.0).asDouble();
    a.operator_id = (*json).get("operator_id", 1).asInt();
    a.latitude = (*json).get("latitude", 0.0).asDouble();
    a.longitude = (*json).get("longitude", 0.0).asDouble();

    AntenneService::createAntenne(a, [callback](const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k201Created);
            resp->setBody("Antenne created");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("DB Error: " + err);
            callback(resp);
        }
    });
}

void AntenneController::getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    AntenneService::getAllAntennes([callback](const std::vector<AntenneModel>& list, const std::string& err) {
        if (err.empty()) {
            Json::Value arr(Json::arrayValue);
            for (const auto& item : list) {
                arr.append(item.toJson());
            }
            auto resp = HttpResponse::newHttpJsonResponse(arr);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}