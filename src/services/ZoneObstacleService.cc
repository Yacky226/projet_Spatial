#include "ZoneObstacleService.h"

using namespace drogon;
using namespace drogon::orm;

// Get all obstacles for a specific zone
void ZoneObstacleService::getObstaclesByZone(int zone_id,
                                              std::function<void(const std::vector<int> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT obstacle_id FROM zone_obstacle WHERE zone_id = $1 ORDER BY obstacle_id";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<int> obstacles;
            for (auto row : r) {
                obstacles.push_back(row["obstacle_id"].as<int>());
            }
            callback(obstacles, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, zone_id);
}

// Get all zones for a specific obstacle
void ZoneObstacleService::getZonesByObstacle(int obstacle_id,
                                              std::function<void(const std::vector<int> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT zone_id FROM zone_obstacle WHERE obstacle_id = $1 ORDER BY zone_id";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<int> zones;
            for (auto row : r) {
                zones.push_back(row["zone_id"].as<int>());
            }
            callback(zones, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, obstacle_id);
}