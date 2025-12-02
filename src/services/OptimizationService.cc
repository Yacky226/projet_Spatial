#include "OptimizationService.h"
#include "../utils/ErrorHandler.h" 
#include <cmath>
#include <algorithm>
#include <random>

using namespace drogon;
using namespace drogon::orm;

void OptimizationService::optimizeGreedy(const OptimizationRequest& req, 
                                         std::function<void(const std::vector<OptimizationResult>&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // ALGORITHME SQL "MONTE CARLO" :
    // 1. On sélectionne la géométrie de la zone cible (zone_id OU bbox).
    // 2. ST_GeneratePoints : On génère 100 points aléatoires DANS la zone.
    // 3. LATERAL JOIN : Pour CHAQUE point candidat, on simule une couverture.
    // 4. ST_Intersection : On calcule la surface couverte par ce candidat.
    // 5. On multiplie par la densité de la zone pour avoir la population estimée.
    // 6. On trie par population décroissante et on garde le top N.

    std::string sql;
    
    if (req.isZoneMode()) {
        // Mode zone_id : utilise la zone prédéfinie
        // Construire la requête SQL avec le nombre d'antennes
        sql = "WITH target_zone AS ("
              "    SELECT geom, density FROM zone WHERE id = $1"
              "), "
              "candidates AS ("
              "    SELECT (ST_Dump(ST_GeneratePoints(geom, 100))).geom as pt_geom"
              "    FROM target_zone"
              ") "
              "SELECT "
              "    ST_X(pt_geom) as lon,"
              "    ST_Y(pt_geom) as lat,"
              "    (ST_Area(ST_Intersection(target_zone.geom, ST_Buffer(pt_geom::geography, $2)::geometry)::geography) / 1000000.0) * target_zone.density as pop_covered "
              "FROM candidates, target_zone "
              "ORDER BY pop_covered DESC "
              "LIMIT " + std::to_string(req.antennas_count);
        
        client->execSqlAsync(
            sql,
            [callback](const Result &r) {
                std::vector<OptimizationResult> results;
                for (auto row : r) {
                    OptimizationResult res;
                    res.longitude = row["lon"].as<double>();
                    res.latitude = row["lat"].as<double>();
                    res.estimated_population = row["pop_covered"].as<double>();
                    res.score = static_cast<int>(res.estimated_population); 
                    results.push_back(res);
                }
                callback(results, "");
            },
            [callback](const DrogonDbException &e) {
                callback({}, e.base().what());
            },
            req.zone_id.value(),
            req.radius
        );
    } else {
        // Mode bbox : utilise la géométrie du bbox directement
        sql = "WITH bbox_geom AS ("
              "    SELECT ST_GeomFromText($1, 4326) as geom"
              "), "
              "target_zone AS ("
              "    SELECT "
              "        bg.geom,"
              "        COALESCE("
              "            (SELECT AVG(density) FROM zone z WHERE ST_Intersects(z.geom, bg.geom)),"
              "            1000.0"
              "        ) as density"
              "    FROM bbox_geom bg"
              "), "
              "candidates AS ("
              "    SELECT (ST_Dump(ST_GeneratePoints(geom, 100))).geom as pt_geom"
              "    FROM target_zone"
              ") "
              "SELECT "
              "    ST_X(pt_geom) as lon,"
              "    ST_Y(pt_geom) as lat,"
              "    (ST_Area(ST_Intersection(target_zone.geom, ST_Buffer(pt_geom::geography, $2)::geometry)::geography) / 1000000.0) * target_zone.density as pop_covered "
              "FROM candidates, target_zone "
              "ORDER BY pop_covered DESC "
              "LIMIT " + std::to_string(req.antennas_count);
        
        client->execSqlAsync(
            sql,
            [callback](const Result &r) {
                std::vector<OptimizationResult> results;
                for (auto row : r) {
                    OptimizationResult res;
                    res.longitude = row["lon"].as<double>();
                    res.latitude = row["lat"].as<double>();
                    res.estimated_population = row["pop_covered"].as<double>();
                    res.score = static_cast<int>(res.estimated_population); 
                    results.push_back(res);
                }
                callback(results, "");
            },
            [callback](const DrogonDbException &e) {
                callback({}, e.base().what());
            },
            req.bbox_wkt.value(),
            req.radius
        );
    }
}

// Helper pour calculer les centroïdes K-means
static void computeKMeansCentroids(
    const drogon::orm::Result& r,
    int K,
    std::vector<std::tuple<double, double, double>>& centroids_out
) {
    struct Point {
        double lat, lon, weight;
    };
    
    std::vector<Point> points;
    for (auto row : r) {
        points.push_back({
            row["lat"].as<double>(),
            row["lon"].as<double>(),
            row["weight"].as<double>()
        });
    }

    std::vector<Point> centroids(K);
    
    // Initialisation : K-means++
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, points.size() - 1);
    
    centroids[0] = points[dis(gen)];
    
    for (int k = 1; k < K; k++) {
        std::vector<double> distances(points.size());
        double sum = 0.0;
        
        for (size_t i = 0; i < points.size(); i++) {
            double minDist = std::numeric_limits<double>::max();
            for (int j = 0; j < k; j++) {
                double dx = points[i].lon - centroids[j].lon;
                double dy = points[i].lat - centroids[j].lat;
                double dist = dx*dx + dy*dy;
                minDist = std::min(minDist, dist);
            }
            distances[i] = minDist;
            sum += minDist;
        }
        
        std::uniform_real_distribution<> prob(0.0, sum);
        double target = prob(gen);
        double cumsum = 0.0;
        
        for (size_t i = 0; i < points.size(); i++) {
            cumsum += distances[i];
            if (cumsum >= target) {
                centroids[k] = points[i];
                break;
            }
        }
    }
    
    // Itérations K-means
    bool changed = true;
    int maxIter = 50;
    std::vector<int> assignments(points.size());
    
    for (int iter = 0; iter < maxIter && changed; iter++) {
        changed = false;
        
        // Assignment step
        for (size_t i = 0; i < points.size(); i++) {
            double minDist = std::numeric_limits<double>::max();
            int bestCluster = 0;
            
            for (int k = 0; k < K; k++) {
                double dx = points[i].lon - centroids[k].lon;
                double dy = points[i].lat - centroids[k].lat;
                double dist = dx*dx + dy*dy;
                
                if (dist < minDist) {
                    minDist = dist;
                    bestCluster = k;
                }
            }
            
            if (assignments[i] != bestCluster) {
                assignments[i] = bestCluster;
                changed = true;
            }
        }
        
        // Update step
        std::vector<double> sumLat(K, 0.0);
        std::vector<double> sumLon(K, 0.0);
        std::vector<double> sumWeight(K, 0.0);
        
        for (size_t i = 0; i < points.size(); i++) {
            int cluster = assignments[i];
            sumLat[cluster] += points[i].lat * points[i].weight;
            sumLon[cluster] += points[i].lon * points[i].weight;
            sumWeight[cluster] += points[i].weight;
        }
        
        for (int k = 0; k < K; k++) {
            if (sumWeight[k] > 0) {
                centroids[k].lat = sumLat[k] / sumWeight[k];
                centroids[k].lon = sumLon[k] / sumWeight[k];
            }
        }
    }
    
    // Convertir en tuples
    for (int k = 0; k < K; k++) {
        centroids_out.push_back(std::make_tuple(centroids[k].lat, centroids[k].lon, centroids[k].weight));
    }
}

// ============================================================================
// NOUVEL ALGORITHME : K-MEANS CLUSTERING
// ============================================================================
void OptimizationService::optimizeKMeans(const OptimizationRequest& req, 
                                         std::function<void(const std::vector<OptimizationResult>&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // Étape 1 : Récupérer tous les points de population dans la zone
    std::string sql;
    
    if (req.isZoneMode()) {
        // Mode zone_id : utilise la zone prédéfinie
        sql = R"(
            WITH zone_data AS (
                SELECT 
                    id,
                    geom,
                    density,
                    ST_Area(geom::geography) / 1000000.0 as area_km2
                FROM zone 
                WHERE id = $1
            ),
            -- Générer une grille de points pondérés par la densité
            population_points AS (
                SELECT 
                    (ST_DumpPoints(
                        ST_GeneratePoints(
                            z.geom, 
                            GREATEST(100, FLOOR(z.density * z.area_km2 / 100))::int
                        )
                    )).geom as point,
                    z.density
                FROM zone_data z
            )
            SELECT 
                ST_X(point) as lon,
                ST_Y(point) as lat,
                density as weight
            FROM population_points
            ORDER BY RANDOM()
            LIMIT 1000
        )";
        
        client->execSqlAsync(sql,
            [callback, req, client](const Result& r) {
                if (r.empty()) {
                    callback({}, "Zone not found or empty");
                    return;
                }

                // Calculer les centroïdes K-means
                std::vector<std::tuple<double, double, double>> centroids;
                computeKMeansCentroids(r, req.antennas_count, centroids);
                
                // Calculer la population couverte par chaque centroïde (mode zone)
                std::string coverageSQL = R"(
                    WITH antenna_coverage AS (
                        SELECT 
                            $1 as lat,
                            $2 as lon,
                            ST_Buffer(
                                ST_SetSRID(ST_MakePoint($2, $1), 4326)::geography,
                                $3
                            )::geometry as coverage
                    ),
                    zone_intersection AS (
                        SELECT 
                            z.density,
                            ST_Area(
                                ST_Intersection(z.geom, ac.coverage)::geography
                            ) / 1000000.0 as covered_area_km2
                        FROM zone z, antenna_coverage ac
                        WHERE z.id = $4 AND ST_Intersects(z.geom, ac.coverage)
                    )
                    SELECT 
                        COALESCE(SUM(density * covered_area_km2), 0)::float as population
                    FROM zone_intersection
                )";
                
                auto results = std::make_shared<std::vector<OptimizationResult>>();
                auto completed = std::make_shared<std::atomic<int>>(0);
                int K = centroids.size();
                
                for (size_t k = 0; k < centroids.size(); k++) {
                    double lat = std::get<0>(centroids[k]);
                    double lon = std::get<1>(centroids[k]);
                    
                    client->execSqlAsync(coverageSQL,
                        [callback, results, completed, K, lat, lon](const Result& r) {
                            OptimizationResult res;
                            res.latitude = lat;
                            res.longitude = lon;
                            res.estimated_population = r[0]["population"].as<double>();
                            res.score = static_cast<int>(res.estimated_population);
                            
                            results->push_back(res);
                            
                            if (++(*completed) == K) {
                                std::sort(results->begin(), results->end(), 
                                    [](const OptimizationResult& a, const OptimizationResult& b) {
                                        return a.estimated_population > b.estimated_population;
                                    });
                                callback(*results, "");
                            }
                        },
                        [callback](const DrogonDbException& e) {
                            callback({}, e.base().what());
                        },
                        lat, lon, req.radius, req.zone_id.value()
                    );
                }
            },
            [callback](const DrogonDbException& e) {
                callback({}, e.base().what());
            },
            req.zone_id.value()
        );
    } else {
        // Mode bbox : utilise la géométrie du bbox
        sql = R"(
            WITH zone_data AS (
                SELECT 
                    ST_GeomFromText($1, 4326) as geom,
                    COALESCE(
                        (SELECT AVG(density) FROM zone WHERE ST_Intersects(geom, ST_GeomFromText($1, 4326))),
                        1000.0
                    ) as density,
                    ST_Area(ST_GeomFromText($1, 4326)::geography) / 1000000.0 as area_km2
            ),
            -- Générer une grille de points pondérés par la densité
            population_points AS (
                SELECT 
                    (ST_DumpPoints(
                        ST_GeneratePoints(
                            z.geom, 
                            GREATEST(100, FLOOR(z.density * z.area_km2 / 100))::int
                        )
                    )).geom as point,
                    z.density
                FROM zone_data z
            )
            SELECT 
                ST_X(point) as lon,
                ST_Y(point) as lat,
                density as weight
            FROM population_points
            ORDER BY RANDOM()
            LIMIT 1000
        )";
        
        client->execSqlAsync(sql,
            [callback, req, client](const Result& r) {
                if (r.empty()) {
                    callback({}, "Zone not found or empty");
                    return;
                }

                // Calculer les centroïdes K-means
                std::vector<std::tuple<double, double, double>> centroids;
                computeKMeansCentroids(r, req.antennas_count, centroids);
                
                // Calculer la population couverte par chaque centroïde (mode bbox)
                std::string coverageSQL = R"(
                    WITH antenna_coverage AS (
                        SELECT 
                            $1 as lat,
                            $2 as lon,
                            ST_Buffer(
                                ST_SetSRID(ST_MakePoint($2, $1), 4326)::geography,
                                $3
                            )::geometry as coverage
                    ),
                    bbox_geom AS (
                        SELECT ST_GeomFromText($4, 4326) as geom
                    ),
                    zone_intersection AS (
                        SELECT 
                            z.density,
                            ST_Area(
                                ST_Intersection(z.geom, ac.coverage)::geography
                            ) / 1000000.0 as covered_area_km2
                        FROM zone z, antenna_coverage ac, bbox_geom bg
                        WHERE ST_Intersects(z.geom, bg.geom) 
                          AND ST_Intersects(z.geom, ac.coverage)
                    )
                    SELECT 
                        COALESCE(SUM(density * covered_area_km2), 0)::float as population
                    FROM zone_intersection
                )";
                
                auto results = std::make_shared<std::vector<OptimizationResult>>();
                auto completed = std::make_shared<std::atomic<int>>(0);
                int K = centroids.size();
                
                for (size_t k = 0; k < centroids.size(); k++) {
                    double lat = std::get<0>(centroids[k]);
                    double lon = std::get<1>(centroids[k]);
                    
                    client->execSqlAsync(coverageSQL,
                        [callback, results, completed, K, lat, lon](const Result& r) {
                            OptimizationResult res;
                            res.latitude = lat;
                            res.longitude = lon;
                            res.estimated_population = r[0]["population"].as<double>();
                            res.score = static_cast<int>(res.estimated_population);
                            
                            results->push_back(res);
                            
                            if (++(*completed) == K) {
                                std::sort(results->begin(), results->end(), 
                                    [](const OptimizationResult& a, const OptimizationResult& b) {
                                        return a.estimated_population > b.estimated_population;
                                    });
                                callback(*results, "");
                            }
                        },
                        [callback](const DrogonDbException& e) {
                            callback({}, e.base().what());
                        },
                        lat, lon, req.radius, req.bbox_wkt.value()
                    );
                }
            },
            [callback](const DrogonDbException& e) {
                callback({}, e.base().what());
            },
            req.bbox_wkt.value()
        );
    }
}