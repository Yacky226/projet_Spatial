#pragma once
#include "../models/Operator.h"
#include <drogon/drogon.h>

class OperatorService {
public:
    static void getAll(std::function<void(const std::vector<OperatorModel>&, const std::string&)> callback);
};