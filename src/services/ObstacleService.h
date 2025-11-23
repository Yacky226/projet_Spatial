#pragma once
#include "../models/Obstacle.h"
#include "../utils/PaginationMeta.h"
#include <drogon/drogon.h>
#include <vector>
#include <string>
#include <functional>

class ObstacleService {
public:
    static void create(const ObstacleModel &obs, std::function<void(const std::string&)> callback);
    static void getAll(std::function<void(const std::vector<ObstacleModel>&, const std::string&)> callback);
    static void getById(int id, std::function<void(const ObstacleModel&, const std::string&)> callback);
    static void update(const ObstacleModel &obs, std::function<void(const std::string&)> callback);
    static void remove(int id, std::function<void(const std::string&)> callback);
    static void getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback);
    
    static void getAllPaginated(
        int page, 
        int pageSize,
        std::function<void(const std::vector<ObstacleModel>&, const PaginationMeta&, const std::string&)> callback
    );
    
    static void getAllGeoJSONPaginated(
        int page,
        int pageSize,
        std::function<void(const Json::Value&, const PaginationMeta&, const std::string&)> callback
    );
};