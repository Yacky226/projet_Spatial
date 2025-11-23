#include "OperatorController.h"

// 1. CREATE (POST /api/operators)
void OperatorController::create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto jsonPtr = req->getJsonObject();
    
    // Validation basique
    if (!jsonPtr || !jsonPtr->isMember("name")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing 'name' field in JSON");
        callback(resp);
        return;
    }

    OperatorModel op;
    op.name = (*jsonPtr)["name"].asString();

    OperatorService::create(op, [callback](const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k201Created);
            resp->setBody("Operator created successfully");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Error: " + err); // Probablement une violation de contrainte UNIQUE sur le nom
            callback(resp);
        }
    });
}

// 2. READ ALL (GET /api/operators)
void OperatorController::getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    OperatorService::getAll([callback](const std::vector<OperatorModel>& list, const std::string& err) {
        if (err.empty()) {
            Json::Value jsonArray(Json::arrayValue);
            for (const auto& op : list) {
                jsonArray.append(op.toJson());
            }
            auto resp = HttpResponse::newHttpJsonResponse(jsonArray);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 3. READ ONE (GET /api/operators/{id})
void OperatorController::getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    OperatorService::getById(id, [callback](const OperatorModel& op, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(op.toJson());
            callback(resp);
        } else {
            // Si erreur, on assume ici que c'est "Not Found" pour simplifier
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            resp->setBody("Operator not found");
            callback(resp);
        }
    });
}

// 4. UPDATE (PUT /api/operators/{id})
void OperatorController::update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    OperatorModel op;
    op.id = id; // On force l'ID de l'URL
    // On met à jour le nom si présent
    if (jsonPtr->isMember("name")) {
        op.name = (*jsonPtr)["name"].asString();
    }

    OperatorService::update(op, [callback](const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setBody("Operator updated");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 5. DELETE (DELETE /api/operators/{id})
void OperatorController::remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id) {
    OperatorService::remove(id, [callback](const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k204NoContent); // 204 = Succès mais pas de contenu à renvoyer
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

