#include "AntenneController.h"

// 1. CREATE
void AntenneController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    Antenna a;
    // Valeurs par défaut et parsing sécurisé
    a.coverage_radius = (*json).get("coverage_radius", 1.0).asDouble();
    a.status = (*json).get("status", "active").asString();
    a.technology = (*json).get("technology", "4G").asString();
    a.installation_date = (*json).get("installation_date", "").asString();
    a.operator_id = (*json).get("operator_id", 0).asInt();
    a.latitude = (*json).get("latitude", 0.0).asDouble();
    a.longitude = (*json).get("longitude", 0.0).asDouble();

    if(a.operator_id == 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("operator_id is required");
        callback(resp);
        return;
    }

    AntenneService::create(a, [callback](const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k201Created);
            resp->setBody("Antenne created");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 2. READ ALL
void AntenneController::getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    AntenneService::getAll([callback](const std::vector<Antenna>& list, const std::string& err) {
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

// 3. READ ONE
void AntenneController::getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    AntenneService::getById(id, [callback](const Antenna& a, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(a.toJson());
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            callback(resp);
        }
    });
}

// 4. UPDATE
void AntenneController::update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    Antenna a;
    a.id = id;
    // On récupère les nouvelles valeurs
    a.coverage_radius = (*json).get("coverage_radius", 0).asDouble();
    a.status = (*json).get("status", "active").asString();
    a.technology = (*json).get("technology", "4G").asString();
    a.installation_date = (*json).get("installation_date", "").asString(); 
    a.operator_id = (*json).get("operator_id", 0).asInt();
    a.latitude = (*json).get("latitude", 0).asDouble();
    a.longitude = (*json).get("longitude", 0).asDouble();

    AntenneService::update(a, [callback](const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setBody("Antenne updated");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 5. DELETE
void AntenneController::remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    AntenneService::remove(id, [callback](const std::string& err) {
        if (err.empty()) {
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

void AntenneController::search(const HttpRequestPtr& req, 
                               std::function<void (const HttpResponsePtr &)> &&callback, 
                               double lat, double lon, double radius) {
    
    // Validation basique
    if (radius <= 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Radius must be positive");
        callback(resp);
        return;
    }

    AntenneService::getInRadius(lat, lon, radius, [callback](const std::vector<Antenna>& list, const std::string& err) {
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

void AntenneController::getGeoJSON(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    AntenneService::getAllGeoJSON([callback](const Json::Value& geojson, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(geojson);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}