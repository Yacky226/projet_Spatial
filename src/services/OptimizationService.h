#pragma once
#include "../models/OptimizationRequest.h"
#include <drogon/drogon.h>
#include <vector>
#include <functional>

class OptimizationService {
public:
    // Algorithme existant
    static void optimizeGreedy(const OptimizationRequest& req, 
                               std::function<void(const std::vector<OptimizationResult>&, const std::string&)> callback);
    
    // NOUVEAU : Algorithme K-means
    static void optimizeKMeans(const OptimizationRequest& req, 
                               std::function<void(const std::vector<OptimizationResult>&, const std::string&)> callback);
};