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
    // Link zone to obstacle
    static void link(int zone_id, int obstacle_id, std::function<void(const std::string &)> callback);

    // Unlink zone from obstacle
    static void unlink(int zone_id, int obstacle_id, std::function<void(const std::string &)> callback);

    // Get all obstacles for a specific zone
    static void getObstaclesByZone(int zone_id,
                                    std::function<void(const std::vector<int> &, const std::string &)> callback);

    // Get all zones for a specific obstacle
    static void getZonesByObstacle(int obstacle_id,
                                    std::function<void(const std::vector<int> &, const std::string &)> callback);

    // Get all zone-obstacle links
    static void getAll(std::function<void(const std::vector<ZoneObstacleLink> &, const std::string &)> callback);

    // Check if zone is linked to obstacle
    static void isLinked(int zone_id, int obstacle_id, std::function<void(bool, const std::string &)> callback);
};