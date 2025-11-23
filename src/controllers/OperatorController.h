#pragma once
#include <drogon/HttpController.h>
#include "../services/OperatorService.h"

using namespace drogon;

class OperatorController : public drogon::HttpController<OperatorController> {
public:
    METHOD_LIST_BEGIN
        // Create
        ADD_METHOD_TO(OperatorController::create, "/api/operators", Post);
        
        // Read All
        ADD_METHOD_TO(OperatorController::getAll, "/api/operators", Get);
        
        // Read One (ex: /api/operators/1)
        ADD_METHOD_TO(OperatorController::getById, "/api/operators/{1}", Get);
        
        // Update
        ADD_METHOD_TO(OperatorController::update, "/api/operators/{1}", Put);
        
        // Delete
        ADD_METHOD_TO(OperatorController::remove, "/api/operators/{1}", Delete);
    METHOD_LIST_END

    void create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getById(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void update(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
    void remove(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int id);
};