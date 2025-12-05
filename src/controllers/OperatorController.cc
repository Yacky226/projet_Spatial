#include "OperatorController.h"

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

