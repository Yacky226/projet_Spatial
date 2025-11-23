#pragma once
#include <drogon/drogon.h>
#include <string>

struct OperatorModel {
    int id = -1;
    std::string name;

    Json::Value toJson() const {
        Json::Value ret;
        if (id != -1) ret["id"] = id;
        ret["name"] = name;
        return ret;
    }
};