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

    // ALGORITHME AMÉLIORÉ :
    // 1. Calculer la couverture ACTUELLE de l'opérateur
    // 2. Identifier les zones NON COUVERTES par cet opérateur
    // 3. Générer des points candidats DANS les zones blanches
    // 4. Calculer le gain de couverture pour chaque candidat

    std::string sql = R"(
        WITH target_zone AS (
            SELECT 
                id,
                geom, 
                density,
                ST_Area(geom::geography) / 1000000.0 as total_area_km2
            FROM zone 
            WHERE id = $1
        ),
        -- Couverture actuelle de l'opérateur dans cette zone
        current_operator_coverage AS (
            SELECT ST_Union(
                ST_Buffer(a.geom::geography, a.coverage_radius)::geometry
            ) as covered_area
            FROM antenna a, target_zone z
            WHERE a.operator_id = $2
              AND a.status = 'active'
              AND ST_DWithin(a.geom::geography, z.geom::geography, a.coverage_radius)
        ),
        -- Zones blanches (non couvertes par CET opérateur)
        white_zones AS (
            SELECT 
                ST_Difference(
                    z.geom,
                    COALESCE(c.covered_area, 'GEOMETRYCOLLECTION EMPTY'::geometry)
                ) as uncovered_geom,
                z.density,
                z.total_area_km2
            FROM target_zone z
            LEFT JOIN current_operator_coverage c ON true
        ),
        -- Générer des points candidats UNIQUEMENT dans les zones blanches
        candidates AS (
            SELECT 
                (ST_Dump(
                    ST_GeneratePoints(
                        CASE 
                            WHEN ST_IsEmpty(uncovered_geom) THEN (SELECT geom FROM target_zone)
                            ELSE uncovered_geom
                        END,
                        200  -- Plus de points pour meilleures suggestions
                    )
                )).geom as pt_geom,
                density,
                ST_Area(uncovered_geom::geography) / 1000000.0 as white_zone_area_km2
            FROM white_zones
            WHERE NOT ST_IsEmpty(uncovered_geom)
        )
        SELECT 
            ST_X(pt_geom) as lon,
            ST_Y(pt_geom) as lat,
            -- Population couverte par ce nouveau point
            (ST_Area(
                ST_Intersection(
                    (SELECT geom FROM target_zone),
                    ST_Buffer(pt_geom::geography, $3)::geometry
                )::geography
            ) / 1000000.0) * density as pop_covered,
            -- Surface de zone blanche qui sera couverte
            (ST_Area(
                ST_Intersection(
                    (SELECT uncovered_geom FROM white_zones),
                    ST_Buffer(pt_geom::geography, $3)::geometry
                )::geography
            ) / 1000000.0) as white_zone_covered_km2,
            -- Pourcentage d'amélioration de couverture
            ((ST_Area(
                ST_Intersection(
                    (SELECT uncovered_geom FROM white_zones),
                    ST_Buffer(pt_geom::geography, $3)::geometry
                )::geography
            ) / 1000000.0) / white_zone_area_km2) * 100 as coverage_improvement_percent
        FROM candidates
        ORDER BY white_zone_covered_km2 DESC, pop_covered DESC
        LIMIT $4
    )";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<OptimizationResult> results;
            for (auto row : r) {
                OptimizationResult res;
                res.longitude = row["lon"].as<double>();
                res.latitude = row["lat"].as<double>();
                res.estimated_population = row["pop_covered"].as<double>();
                res.uncovered_area_km2 = row["white_zone_covered_km2"].as<double>();
                res.coverage_improvement = row["coverage_improvement_percent"].as<double>();
                
                // Score basé sur l'amélioration de la couverture
                res.score = static_cast<int>(res.coverage_improvement * 10); 
                results.push_back(res);
            }
            callback(results, "");
        },
        [callback](const DrogonDbException &e) {
            LOG_ERROR << "OptimizationService::optimizeGreedy - Error: " << e.base().what();
            callback({}, e.base().what());
        },
        req.zone_id,
        req.operator_id,  // 🆕 Filtrer par opérateur
        req.radius,
        req.antennas_count
    );
}

// ============================================================================
// NOUVEL ALGORITHME : K-MEANS CLUSTERING
// ============================================================================
void OptimizationService::optimizeKMeans(const OptimizationRequest& req, 
                                         std::function<void(const std::vector<OptimizationResult>&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // Étape 1 : Récupérer les zones blanches de l'opérateur et générer des points
    std::string sql = R"(
        WITH target_zone AS (
            SELECT 
                id,
                geom,
                density,
                ST_Area(geom::geography) / 1000000.0 as area_km2
            FROM zone 
            WHERE id = $1
        ),
        -- Couverture actuelle de l'opérateur
        operator_coverage AS (
            SELECT ST_Union(
                ST_Buffer(a.geom::geography, a.coverage_radius)::geometry
            ) as covered_area
            FROM antenna a, target_zone z
            WHERE a.operator_id = $2
              AND a.status = 'active'
              AND ST_DWithin(a.geom::geography, z.geom::geography, a.coverage_radius)
        ),
        -- Zones blanches de cet opérateur
        white_zones AS (
            SELECT 
                ST_Difference(
                    z.geom,
                    COALESCE(c.covered_area, 'GEOMETRYCOLLECTION EMPTY'::geometry)
                ) as uncovered_geom,
                z.density
            FROM target_zone z
            LEFT JOIN operator_coverage c ON true
        ),
        -- Générer des points pondérés dans les zones blanches
        population_points AS (
            SELECT 
                (ST_DumpPoints(
                    ST_GeneratePoints(
                        CASE 
                            WHEN ST_IsEmpty(uncovered_geom) THEN (SELECT geom FROM target_zone)
                            ELSE uncovered_geom
                        END,
                        GREATEST(200, FLOOR(density * 10))::int
                    )
                )).geom as point,
                density as weight
            FROM white_zones
        )
        SELECT 
            ST_X(point) as lon,
            ST_Y(point) as lat,
            weight
        FROM population_points
        ORDER BY RANDOM()
        LIMIT 1000
    )";

    client->execSqlAsync(sql,
        [callback, req, client](const Result& r) {
            if (r.empty()) {
                callback({}, "Zone not found or already 100% covered by this operator");
                return;
            }

            // Étape 2 : K-means en C++
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

            int K = req.antennas_count;
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
            
            // Étape 3 : Calculer la couverture en tenant compte de l'opérateur existant
            std::string coverageSQL = R"(
                WITH antenna_coverage AS (
                    SELECT 
                        $1 as lat,
                        $2 as lon,
                        ST_Buffer(
                            ST_SetSRID(ST_MakePoint($2, $1), 4326)::geography,
                            $3
                        )::geometry as new_coverage
                ),
                operator_white_zones AS (
                    SELECT 
                        z.density,
                        ST_Difference(
                            z.geom,
                            COALESCE(
                                (SELECT ST_Union(ST_Buffer(a.geom::geography, a.coverage_radius)::geometry)
                                 FROM antenna a
                                 WHERE a.operator_id = $5 AND a.status = 'active'),
                                'GEOMETRYCOLLECTION EMPTY'::geometry
                            )
                        ) as uncovered
                    FROM zone z
                    WHERE z.id = $4
                )
                SELECT 
                    COALESCE(
                        SUM(
                            (ST_Area(
                                ST_Intersection(w.uncovered, ac.new_coverage)::geography
                            ) / 1000000.0) * w.density
                        ), 
                        0
                    )::float as population,
                    COALESCE(
                        SUM(
                            ST_Area(
                                ST_Intersection(w.uncovered, ac.new_coverage)::geography
                            ) / 1000000.0
                        ),
                        0
                    )::float as white_zone_covered_km2
                FROM operator_white_zones w, antenna_coverage ac
            )";
            
            std::vector<OptimizationResult> results;
            std::atomic<int> completed(0);
            double total_white_zone_area = 0.0;
            
            for (int k = 0; k < K; k++) {
                client->execSqlAsync(coverageSQL,
                    [callback, &results, &completed, K, centroids, k, &total_white_zone_area](const Result& r) {
                        OptimizationResult res;
                        res.latitude = centroids[k].lat;
                        res.longitude = centroids[k].lon;
                        res.estimated_population = r[0]["population"].as<double>();
                        res.uncovered_area_km2 = r[0]["white_zone_covered_km2"].as<double>();
                        res.coverage_improvement = (res.uncovered_area_km2 / total_white_zone_area) * 100;
                        res.score = static_cast<int>(res.estimated_population);
                        
                        results.push_back(res);
                        
                        if (++completed == K) {
                            std::sort(results.begin(), results.end(), 
                                [](const OptimizationResult& a, const OptimizationResult& b) {
                                    return a.uncovered_area_km2 > b.uncovered_area_km2;
                                });
                            callback(results, "");
                        }
                    },
                    [callback](const DrogonDbException& e) {
                        callback({}, e.base().what());
                    },
                    centroids[k].lat,
                    centroids[k].lon,
                    req.radius,
                    req.zone_id,
                    req.operator_id  // 🆕 Paramètre opérateur
                );
            }
        },
        [callback](const DrogonDbException& e) {
            LOG_ERROR << "OptimizationService::optimizeKMeans - Error: " << e.base().what();
            callback({}, e.base().what());
        },
        req.zone_id,
        req.operator_id  // 🆕 Paramètre opérateur
    );
}