#pragma once

#include <drogon/HttpAppFramework.h>
#include <functional>
#include <vector>

struct ZoneObstacleLink {
    int zone_id;
    int obstacle_id;
};

class ZoneObstacleService {
public:
    // Get all obstacles for a specific zone
    static void getObstaclesByZone(int zone_id,
                                    std::function<void(const std::vector<int> &, const std::string &)> callback);

    // Get all zones for a specific obstacle
    static void getZonesByObstacle(int obstacle_id,
                                    std::function<void(const std::vector<int> &, const std::string &)> callback);
};