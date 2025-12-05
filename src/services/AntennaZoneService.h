#pragma once

#include <drogon/HttpAppFramework.h>
#include <functional>
#include <vector>

class AntennaZoneService {
public:
    // Get all antennas for a specific zone
    static void getAntennasByZone(int zone_id,
                                   std::function<void(const std::vector<int> &, const std::string &)> callback);
};