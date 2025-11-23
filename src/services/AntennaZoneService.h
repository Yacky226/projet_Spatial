#pragma once

#include <drogon/HttpAppFramework.h>
#include <functional>
#include <vector>

struct AntennaZoneLink {
    int antenna_id;
    int zone_id;
};

class AntennaZoneService {
public:
    // Link antenna to zone
    static void link(int antenna_id, int zone_id, std::function<void(const std::string &)> callback);

    // Unlink antenna from zone
    static void unlink(int antenna_id, int zone_id, std::function<void(const std::string &)> callback);

    // Get all zones for a specific antenna
    static void getZonesByAntenna(int antenna_id,
                                   std::function<void(const std::vector<int> &, const std::string &)> callback);

    // Get all antennas for a specific zone
    static void getAntennasByZone(int zone_id,
                                   std::function<void(const std::vector<int> &, const std::string &)> callback);

    // Get all antenna-zone links
    static void getAll(std::function<void(const std::vector<AntennaZoneLink> &, const std::string &)> callback);

    // Check if antenna is linked to zone
    static void isLinked(int antenna_id, int zone_id,
                         std::function<void(bool, const std::string &)> callback);
};