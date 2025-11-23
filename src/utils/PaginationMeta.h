#pragma once
#include <json/json.h>

struct PaginationMeta {
    int currentPage;
    int pageSize;
    int totalItems;
    int totalPages;
    bool hasNext;
    bool hasPrev;
    
    Json::Value toJson() const {
        Json::Value json;
        json["currentPage"] = currentPage;
        json["pageSize"] = pageSize;
        json["totalItems"] = totalItems;
        json["totalPages"] = totalPages;
        json["hasNext"] = hasNext;
        json["hasPrev"] = hasPrev;
        return json;
    }
};