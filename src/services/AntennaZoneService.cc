#include "AntennaZoneService.h"

using namespace drogon;
using namespace drogon::orm;

// Link antenna to zone
void AntennaZoneService::link(int antenna_id, int zone_id, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES ($1, $2) "
                      "ON CONFLICT DO NOTHING";

    client->execSqlAsync(sql,
                         [callback](const Result &) { callback(""); },
                         [callback](const DrogonDbException &e) { callback(e.base().what()); }, antenna_id,
                         zone_id);
}

// Unlink antenna from zone
void AntennaZoneService::unlink(int antenna_id, int zone_id, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "DELETE FROM antenna_zone WHERE antenna_id = $1 AND zone_id = $2";

    client->execSqlAsync(sql,
                         [callback](const Result &) { callback(""); },
                         [callback](const DrogonDbException &e) { callback(e.base().what()); }, antenna_id,
                         zone_id);
}

// Get all zones for a specific antenna
void AntennaZoneService::getZonesByAntenna(int antenna_id,
                                            std::function<void(const std::vector<int> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT zone_id FROM antenna_zone WHERE antenna_id = $1 ORDER BY zone_id";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<int> zones;
            for (auto row : r) {
                zones.push_back(row["zone_id"].as<int>());
            }
            callback(zones, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, antenna_id);
}

// Get all antennas for a specific zone
void AntennaZoneService::getAntennasByZone(int zone_id,
                                            std::function<void(const std::vector<int> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT antenna_id FROM antenna_zone WHERE zone_id = $1 ORDER BY antenna_id";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<int> antennas;
            for (auto row : r) {
                antennas.push_back(row["antenna_id"].as<int>());
            }
            callback(antennas, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, zone_id);
}

// Get all antenna-zone links
void AntennaZoneService::getAll(std::function<void(const std::vector<AntennaZoneLink> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT antenna_id, zone_id FROM antenna_zone ORDER BY antenna_id, zone_id";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<AntennaZoneLink> links;
            for (auto row : r) {
                AntennaZoneLink link;
                link.antenna_id = row["antenna_id"].as<int>();
                link.zone_id = row["zone_id"].as<int>();
                links.push_back(link);
            }
            callback(links, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); });
}

// Check if antenna is linked to zone
void AntennaZoneService::isLinked(int antenna_id, int zone_id,
                                   std::function<void(bool, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT 1 FROM antenna_zone WHERE antenna_id = $1 AND zone_id = $2";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) { callback(!r.empty(), ""); },
        [callback](const DrogonDbException &e) { callback(false, e.base().what()); }, antenna_id, zone_id);
}