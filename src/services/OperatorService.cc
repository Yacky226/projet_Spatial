#include "OperatorService.h"
using namespace drogon;
using namespace drogon::orm;

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