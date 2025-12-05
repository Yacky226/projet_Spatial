#pragma once
#include <drogon/drogon.h>
#include <vector>
#include <string>
#include <functional>
#include <optional>

class ObstacleService {
public:
    // GET OBSTACLES BY BOUNDING BOX
    static void getByBoundingBox(double minLon, double minLat, double maxLon, double maxLat, int zoom, const std::optional<std::string>& type, const std::function<void(const Json::Value&, const std::string&)>& callback);
};