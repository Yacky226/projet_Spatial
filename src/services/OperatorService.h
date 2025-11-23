#pragma once
#include "../models/Operator.h"
#include <drogon/drogon.h>

class OperatorService {
public:
    static void create(const OperatorModel &op, std::function<void(const std::string&)> callback);
    static void getAll(std::function<void(const std::vector<OperatorModel>&, const std::string&)> callback);
    static void getById(int id, std::function<void(const OperatorModel&, const std::string&)> callback);
    static void update(const OperatorModel &op, std::function<void(const std::string&)> callback);
    static void remove(int id, std::function<void(const std::string&)> callback);
};