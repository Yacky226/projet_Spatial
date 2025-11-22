#pragma once
#include <drogon/HttpController.h>
#include "../services/AntenneService.h"

using namespace drogon;

class AntenneController : public drogon::HttpController<AntenneController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AntenneController::create, "/api/antennes", Post);
        ADD_METHOD_TO(AntenneController::getAll, "/api/antennes", Get);
    METHOD_LIST_END

    void create(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
    void getAll(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
};