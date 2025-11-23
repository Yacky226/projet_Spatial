#include "ZoneObstacleService.h"

using namespace drogon;
using namespace drogon::orm;

// Link zone to obstacle
void ZoneObstacleService::link(int zone_id, int obstacle_id, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "INSERT INTO zone_obstacle (zone_id, obstacle_id) VALUES ($1, $2) "
                      "ON CONFLICT DO NOTHING";

    client->execSqlAsync(sql,
                         [callback](const Result &) { callback(""); },
                         [callback](const DrogonDbException &e) { callback(e.base().what()); }, zone_id,
                         obstacle_id);
}

// Unlink zone from obstacle
void ZoneObstacleService::unlink(int zone_id, int obstacle_id, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "DELETE FROM zone_obstacle WHERE zone_id = $1 AND obstacle_id = $2";

    client->execSqlAsync(sql,
                         [callback](const Result &) { callback(""); },
                         [callback](const DrogonDbException &e) { callback(e.base().what()); }, zone_id,
                         obstacle_id);
}

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

// Get all zone-obstacle links
void ZoneObstacleService::getAll(std::function<void(const std::vector<ZoneObstacleLink> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT zone_id, obstacle_id FROM zone_obstacle ORDER BY zone_id, obstacle_id";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<ZoneObstacleLink> links;
            for (auto row : r) {
                ZoneObstacleLink link;
                link.zone_id = row["zone_id"].as<int>();
                link.obstacle_id = row["obstacle_id"].as<int>();
                links.push_back(link);
            }
            callback(links, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); });
}

// Check if zone is linked to obstacle
void ZoneObstacleService::isLinked(int zone_id, int obstacle_id,
                                    std::function<void(bool, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT 1 FROM zone_obstacle WHERE zone_id = $1 AND obstacle_id = $2";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) { callback(!r.empty(), ""); },
        [callback](const DrogonDbException &e) { callback(false, e.base().what()); }, zone_id, obstacle_id);
}