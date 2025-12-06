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

    // Algorithme glouton optimisé pour le placement d'antennes :
    // - Utilise les zones de densité existantes (cellules de 250m)
    // - Ces zones ont déjà une densité de population calculée
    // - Filtre les positions sur obstacles majeurs
    // - Retourne les meilleures positions par couverture de population

    std::string sql;
    
    if (req.isZoneMode()) {
        LOG_INFO << "🎯 Starting Greedy optimization for zone_id=" << req.zone_id.value();
        
        // Requête SQL complexe utilisant les density_zones existantes pour rapidité
        // Fallback sur génération de points si pas de données de densité
        sql = R"(
            WITH target_zone AS (
                SELECT id, geom, COALESCE(density, 100.0) as density
                FROM zone 
                WHERE id = $1
            ),
            -- Utiliser les density_zones enfants si disponibles
            density_cells AS (
                SELECT 
                    ST_Centroid(dz.geom) as pt,
                    dz.density
                FROM zone dz
                WHERE dz.type = 'density_zone'
                  AND dz.parent_id IN (
                      SELECT z.id FROM zone z, target_zone t 
                      WHERE ST_Intersects(z.geom, t.geom)
                        AND z.type IN ('commune', 'province', 'region')
                  )
                ORDER BY dz.density DESC NULLS LAST
                LIMIT 200
            ),
            -- Si pas de density_zones, générer des points sur la zone simplifiée
            fallback_points AS (
                SELECT 
                    (ST_Dump(ST_GeneratePoints(ST_Simplify(t.geom, 0.01), 50))).geom as pt,
                    t.density
                FROM target_zone t
                WHERE NOT EXISTS (SELECT 1 FROM density_cells LIMIT 1)
            ),
            all_candidates AS (
                SELECT pt, density FROM density_cells
                UNION ALL
                SELECT pt, density FROM fallback_points
            ),
            -- Filtrer les points qui tombent sur des obstacles de type polygon (buildings, water)
            valid_candidates AS (
                SELECT c.pt, c.density
                FROM all_candidates c
                WHERE NOT EXISTS (
                    SELECT 1 FROM obstacle o 
                    WHERE o.geom_type IN ('POLYGON', 'MULTIPOLYGON')
                      AND ST_Intersects(c.pt, o.geom)
                    LIMIT 1
                )
            )
            SELECT 
                ST_X(pt) as lon,
                ST_Y(pt) as lat,
                COALESCE(density, 100.0) * 
                    (ST_Area(ST_Buffer(pt::geography, $2)::geometry::geography) / 1000000.0) 
                    as pop_covered
            FROM valid_candidates
            ORDER BY pop_covered DESC
            LIMIT )" + std::to_string(req.antennas_count);
        
        client->execSqlAsync(
            sql,
            [callback](const Result &r) {
                LOG_INFO << "🎯 Greedy completed: " << r.size() << " candidates found";
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
                LOG_ERROR << "🎯 Greedy error: " << e.base().what();
                callback({}, e.base().what());
            },
            req.zone_id.value(),
            req.radius
        );
    } else {
        // Mode bbox
        LOG_INFO << "🎯 Starting Greedy optimization for bbox";
        sql = R"(
            WITH bbox_geom AS (
                SELECT ST_GeomFromText($1, 4326) as geom
            ),
            -- Chercher les density_zones dans le bbox
            density_cells AS (
                SELECT 
                    ST_Centroid(dz.geom) as pt,
                    dz.density
                FROM zone dz, bbox_geom bg
                WHERE dz.type = 'density_zone'
                  AND ST_Intersects(dz.geom, bg.geom)
                ORDER BY dz.density DESC NULLS LAST
                LIMIT 200
            ),
            -- Fallback: générer des points
            fallback_points AS (
                SELECT 
                    (ST_Dump(ST_GeneratePoints(bg.geom, 50))).geom as pt,
                    (SELECT COALESCE(AVG(density), 100.0) FROM zone z WHERE ST_Intersects(z.geom, bg.geom) AND z.density IS NOT NULL) as density
                FROM bbox_geom bg
                WHERE NOT EXISTS (SELECT 1 FROM density_cells LIMIT 1)
            ),
            all_candidates AS (
                SELECT pt, density FROM density_cells
                UNION ALL
                SELECT pt, density FROM fallback_points
            ),
            valid_candidates AS (
                SELECT c.pt, c.density
                FROM all_candidates c
                WHERE NOT EXISTS (
                    SELECT 1 FROM obstacle o 
                    WHERE o.geom_type IN ('POLYGON', 'MULTIPOLYGON')
                      AND ST_Intersects(c.pt, o.geom)
                    LIMIT 1
                )
            )
            SELECT 
                ST_X(pt) as lon,
                ST_Y(pt) as lat,
                COALESCE(density, 100.0) * 
                    (ST_Area(ST_Buffer(pt::geography, $2)::geometry::geography) / 1000000.0) 
                    as pop_covered
            FROM valid_candidates
            ORDER BY pop_covered DESC
            LIMIT )" + std::to_string(req.antennas_count);
        
        client->execSqlAsync(
            sql,
            [callback](const Result &r) {
                LOG_INFO << "🎯 Greedy (bbox) completed: " << r.size() << " candidates";
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
                LOG_ERROR << "🎯 Greedy (bbox) error: " << e.base().what();
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
    // Implémentation de l'algorithme K-means avec initialisation K-means++
    // Pondère les centroïdes par la densité de population pour optimisation
    
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
    
    // Initialisation intelligente K-means++ pour éviter les clusters vides
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, points.size() - 1);
    
    centroids[0] = points[dis(gen)];
    
    // Sélection des centroïdes suivants selon la probabilité basée sur la distance
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
    
    // Itérations principales de l'algorithme K-means (max 50 itérations)
    bool changed = true;
    int maxIter = 50;
    std::vector<int> assignments(points.size());
    
    for (int iter = 0; iter < maxIter && changed; iter++) {
        changed = false;
        
        // Étape d'assignation : chaque point va au centroïde le plus proche
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
        
        // Étape de mise à jour : recalculer les centroïdes pondérés par la densité
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
    
    // Conversion des centroïdes en format de sortie
    for (int k = 0; k < K; k++) {
        centroids_out.push_back(std::make_tuple(centroids[k].lat, centroids[k].lon, centroids[k].weight));
    }
}

// ============================================================================
// ALGORITHME K-MEANS CLUSTERING OPTIMISÉ
// ============================================================================
void OptimizationService::optimizeKMeans(const OptimizationRequest& req, 
                                         std::function<void(const std::vector<OptimizationResult>&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // Algorithme K-means pour optimisation du placement d'antennes
    // Utilise les données de densité existantes comme points d'entrée

    std::string sql;
    
    if (req.isZoneMode()) {
        LOG_INFO << "🎯 Starting K-Means optimization for zone_id=" << req.zone_id.value();
        
        sql = R"(
            WITH target_zone AS (
                SELECT id, geom, COALESCE(density, 100.0) as density
                FROM zone 
                WHERE id = $1
            ),
            -- Récupérer les density_zones dans la zone cible
            density_cells AS (
                SELECT 
                    ST_X(ST_Centroid(dz.geom)) as lon,
                    ST_Y(ST_Centroid(dz.geom)) as lat,
                    COALESCE(dz.density, 100.0) as weight
                FROM zone dz
                WHERE dz.type = 'density_zone'
                  AND dz.parent_id IN (
                      SELECT z.id FROM zone z, target_zone t 
                      WHERE ST_Intersects(z.geom, t.geom)
                        AND z.type IN ('commune', 'province', 'region')
                  )
                ORDER BY dz.density DESC NULLS LAST
                LIMIT 500
            ),
            -- Fallback si pas de density_zones
            fallback_points AS (
                SELECT 
                    ST_X((ST_Dump(ST_GeneratePoints(ST_Simplify(t.geom, 0.01), 100))).geom) as lon,
                    ST_Y((ST_Dump(ST_GeneratePoints(ST_Simplify(t.geom, 0.01), 100))).geom) as lat,
                    t.density as weight
                FROM target_zone t
                WHERE NOT EXISTS (SELECT 1 FROM density_cells LIMIT 1)
            )
            SELECT lon, lat, COALESCE(weight, 100.0) as weight 
            FROM density_cells
            UNION ALL
            SELECT lon, lat, weight FROM fallback_points
        )";
        
        client->execSqlAsync(sql,
            [callback, req, client](const Result& r) {
                if (r.empty()) {
                    LOG_WARN << "🎯 K-Means: No data found for zone";
                    callback({}, "Zone not found or no density data available");
                    return;
                }

                LOG_INFO << "🎯 K-Means: Processing " << r.size() << " data points";

                // Calculer les centroïdes K-means
                std::vector<std::tuple<double, double, double>> centroids;
                computeKMeansCentroids(r, req.antennas_count, centroids);
                
                // Créer les résultats directement (sans recalcul de couverture pour la rapidité)
                std::vector<OptimizationResult> results;
                double radius = req.radius;
                double coverage_area_km2 = (3.14159 * radius * radius) / 1000000.0;
                
                for (size_t k = 0; k < centroids.size(); k++) {
                    OptimizationResult res;
                    res.latitude = std::get<0>(centroids[k]);
                    res.longitude = std::get<1>(centroids[k]);
                    double weight = std::get<2>(centroids[k]);
                    res.estimated_population = weight * coverage_area_km2;
                    res.score = static_cast<int>(res.estimated_population);
                    results.push_back(res);
                }
                
                std::sort(results.begin(), results.end(), 
                    [](const OptimizationResult& a, const OptimizationResult& b) {
                        return a.estimated_population > b.estimated_population;
                    });
                
                LOG_INFO << "🎯 K-Means completed: " << results.size() << " antennas positioned";
                callback(results, "");
            },
            [callback](const DrogonDbException& e) {
                LOG_ERROR << "🎯 K-Means error: " << e.base().what();
                callback({}, e.base().what());
            },
            req.zone_id.value()
        );
    } else {
        // Mode bbox
        LOG_INFO << "🎯 Starting K-Means optimization for bbox";
        
        sql = R"(
            WITH bbox_geom AS (
                SELECT ST_GeomFromText($1, 4326) as geom
            ),
            density_cells AS (
                SELECT 
                    ST_X(ST_Centroid(dz.geom)) as lon,
                    ST_Y(ST_Centroid(dz.geom)) as lat,
                    COALESCE(dz.density, 100.0) as weight
                FROM zone dz, bbox_geom bg
                WHERE dz.type = 'density_zone'
                  AND ST_Intersects(dz.geom, bg.geom)
                ORDER BY dz.density DESC NULLS LAST
                LIMIT 500
            ),
            fallback_points AS (
                SELECT 
                    ST_X((ST_Dump(ST_GeneratePoints(bg.geom, 100))).geom) as lon,
                    ST_Y((ST_Dump(ST_GeneratePoints(bg.geom, 100))).geom) as lat,
                    (SELECT COALESCE(AVG(density), 100.0) FROM zone z WHERE ST_Intersects(z.geom, bg.geom) AND z.density IS NOT NULL) as weight
                FROM bbox_geom bg
                WHERE NOT EXISTS (SELECT 1 FROM density_cells LIMIT 1)
            )
            SELECT lon, lat, COALESCE(weight, 100.0) as weight 
            FROM density_cells
            UNION ALL
            SELECT lon, lat, weight FROM fallback_points
        )";
        
        client->execSqlAsync(sql,
            [callback, req](const Result& r) {
                if (r.empty()) {
                    LOG_WARN << "🎯 K-Means (bbox): No data found";
                    callback({}, "No density data available in bbox");
                    return;
                }

                LOG_INFO << "🎯 K-Means (bbox): Processing " << r.size() << " data points";

                std::vector<std::tuple<double, double, double>> centroids;
                computeKMeansCentroids(r, req.antennas_count, centroids);
                
                std::vector<OptimizationResult> results;
                double radius = req.radius;
                double coverage_area_km2 = (3.14159 * radius * radius) / 1000000.0;
                
                for (size_t k = 0; k < centroids.size(); k++) {
                    OptimizationResult res;
                    res.latitude = std::get<0>(centroids[k]);
                    res.longitude = std::get<1>(centroids[k]);
                    double weight = std::get<2>(centroids[k]);
                    res.estimated_population = weight * coverage_area_km2;
                    res.score = static_cast<int>(res.estimated_population);
                    results.push_back(res);
                }
                
                std::sort(results.begin(), results.end(), 
                    [](const OptimizationResult& a, const OptimizationResult& b) {
                        return a.estimated_population > b.estimated_population;
                    });
                
                LOG_INFO << "🎯 K-Means (bbox) completed: " << results.size() << " antennas";
                callback(results, "");
            },
            [callback](const DrogonDbException& e) {
                LOG_ERROR << "🎯 K-Means (bbox) error: " << e.base().what();
                callback({}, e.base().what());
            },
            req.bbox_wkt.value()
        );
    }
}