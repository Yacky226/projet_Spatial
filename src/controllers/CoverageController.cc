#include "CoverageController.h"

using namespace drogon;
using namespace drogon::orm;

// 1. Couverture spécifique d'une antenne
void CoverageController::getAntennaCoverage(const HttpRequestPtr& req, 
                                            std::function<void (const HttpResponsePtr &)> &&callback, 
                                            int antennaId) {
    
    CoverageService::calculateForAntenna(antennaId, 
        [callback](const CoverageResult& res, const std::string& err) {
            if (err.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(res.toJson());
                callback(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(err);
                callback(resp);
            }
        }
    );
}

// 2. Récupération des zones blanches (GeoJSON)
void CoverageController::getWhiteZones(const HttpRequestPtr& req,
                                     std::function<void(const HttpResponsePtr&)>&& callback,
                                     int zone_id) {
    CoverageService::getWhiteZones(zone_id, [callback](const Json::Value& result, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(result);
            resp->addHeader("Content-Type", "application/geo+json");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(err);
            callback(resp);
        }
    });
}

// 3. Statistiques Globales
void CoverageController::getGlobalStats(const HttpRequestPtr& req,
                                      std::function<void(const HttpResponsePtr&)>&& callback) {
    auto client = app().getDbClient();
    
    // Requête SQL complexe pour les stats globales
    std::string sql = R"(
        WITH coverage_stats AS (
            SELECT 
                COUNT(DISTINCT a.id) as total_antennas,
                COUNT(DISTINCT CASE WHEN a.status = 'active' THEN a.id END) as active_antennas,
                COUNT(DISTINCT CASE WHEN a.technology = '5G' THEN a.id END) as antennas_5g,
                COUNT(DISTINCT CASE WHEN a.technology = '4G' THEN a.id END) as antennas_4g
            FROM antenna a
        ),
        zone_stats AS (
            SELECT 
                COALESCE(SUM(z.population), 0) as total_population,
                COALESCE(SUM(ST_Area(z.geom::geography)) / 1000000.0, 0) as total_area_km2
            FROM zone z
        ),
        coverage_calculation AS (
            SELECT 
                COALESCE(SUM(
                    ST_Area(
                        ST_Intersection(
                            z.geom,
                            ST_Buffer(a.geom::geography, a.coverage_radius)::geometry
                        )::geography
                    ) * z.density / 1000000.0
                ), 0) as covered_population
            FROM antenna a
            CROSS JOIN zone z
            WHERE a.status = 'active'
              AND ST_Intersects(z.geom, ST_Buffer(a.geom::geography, a.coverage_radius)::geometry)
        )
        SELECT json_build_object(
            'antennas', json_build_object(
                'total', c.total_antennas,
                'active', c.active_antennas,
                'by_technology', json_build_object(
                    '5G', c.antennas_5g,
                    '4G', c.antennas_4g
                )
            ),
            'coverage', json_build_object(
                'total_population', z.total_population,
                'covered_population', cc.covered_population,
                'coverage_percent', CASE 
                    WHEN z.total_population > 0 
                    THEN (cc.covered_population / z.total_population * 100)
                    ELSE 0 
                END,
                'total_area_km2', z.total_area_km2
            )
        ) as stats
        FROM coverage_stats c, zone_stats z, coverage_calculation cc
    )";
    
    client->execSqlAsync(sql,
        [callback](const Result& r) {
            if (!r.empty()) {
                std::string jsonStr = r[0]["stats"].as<std::string>();
                Json::Value stats;
                Json::CharReaderBuilder builder;
                std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                std::string errs;

                if (reader->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.length(), &stats, &errs)) {
                    Json::Value response;
                    response["success"] = true;
                    response["data"] = stats;
                    response["timestamp"] = trantor::Date::now().toFormattedString(false);
                    
                    auto resp = HttpResponse::newHttpJsonResponse(response);
                    callback(resp);
                } else {
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k500InternalServerError);
                    resp->setBody("JSON parsing error");
                    callback(resp);
                }
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody("Failed to compute stats");
                callback(resp);
            }
        },
        [callback](const DrogonDbException& e) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody(e.base().what());
            callback(resp);
        }
    );
}