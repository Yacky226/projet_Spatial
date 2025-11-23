#pragma once
#include <drogon/HttpRequest.h>
#include <json/json.h>
#include "PaginationMeta.h"
#include <string>

struct PaginationParams {
    int page = 1;
    int pageSize = 20;
    bool usePagination = false;
};

PaginationParams parsePaginationParams(const drogon::HttpRequestPtr& req);
void addPaginationLinks(Json::Value& response, const PaginationMeta& meta, const std::string& baseUrl);