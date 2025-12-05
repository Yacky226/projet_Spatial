#include "AntennaZoneService.h"

using namespace drogon;
using namespace drogon::orm;

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