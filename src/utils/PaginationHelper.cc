#include "PaginationHelper.h"

PaginationParams parsePaginationParams(const drogon::HttpRequestPtr& req) {
    PaginationParams params;
    auto queryParams = req->getParameters();
    
    if (queryParams.find("page") != queryParams.end()) {
        try {
            params.page = std::stoi(queryParams.at("page"));
            params.usePagination = true;
        } catch (...) { 
            params.page = 1; 
        }
    }
    
    if (queryParams.find("pageSize") != queryParams.end()) {
        try {
            params.pageSize = std::stoi(queryParams.at("pageSize"));
            params.usePagination = true;
        } catch (...) { 
            params.pageSize = 20; 
        }
    }
    
    return params;
}

void addPaginationLinks(Json::Value& response, const PaginationMeta& meta, const std::string& baseUrl) {
    Json::Value links;
    std::string pageSizeParam = "pageSize=" + std::to_string(meta.pageSize);
    
    links["self"] = baseUrl + "?" + pageSizeParam + "&page=" + std::to_string(meta.currentPage);
    links["first"] = baseUrl + "?" + pageSizeParam + "&page=1";
    links["last"] = baseUrl + "?" + pageSizeParam + "&page=" + std::to_string(meta.totalPages);
    
    if (meta.hasNext) {
        links["next"] = baseUrl + "?" + pageSizeParam + "&page=" + std::to_string(meta.currentPage + 1);
    }
    if (meta.hasPrev) {
        links["prev"] = baseUrl + "?" + pageSizeParam + "&page=" + std::to_string(meta.currentPage - 1);
    }
    
    response["pagination"]["links"] = links;
}