#include "OperatorService.h"
using namespace drogon;
using namespace drogon::orm;

void OperatorService::create(const OperatorModel &op, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("INSERT INTO operator (name) VALUES ($1)",
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        op.name);
}

void OperatorService::getAll(std::function<void(const std::vector<OperatorModel>&, const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("SELECT id, name FROM operator ORDER BY id",
        [callback](const Result &r) {
            std::vector<OperatorModel> list;
            for (auto row : r) {
                OperatorModel o;
                o.id = row["id"].as<int>();
                o.name = row["name"].as<std::string>();
                list.push_back(o);
            }
            callback(list, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); });
}

void OperatorService::getById(int id, std::function<void(const OperatorModel&, const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("SELECT id, name FROM operator WHERE id = $1",
        [callback](const Result &r) {
            if (r.size() == 0) { callback({}, "Not Found"); return; }
            OperatorModel o;
            o.id = r[0]["id"].as<int>();
            o.name = r[0]["name"].as<std::string>();
            callback(o, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); },
        id);
}

void OperatorService::update(const OperatorModel &op, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("UPDATE operator SET name = $1 WHERE id = $2",
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        op.name, op.id);
}

void OperatorService::remove(int id, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("DELETE FROM operator WHERE id = $1",
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        id);
}