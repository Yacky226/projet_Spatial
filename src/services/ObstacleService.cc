#include "ObstacleService.h"
#include "../utils/ErrorHandler.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <memory>
#include <optional>

void ObstacleService::getByBoundingBox(double minLon, double minLat, double maxLon, double maxLat, int zoom, const std::optional<std::string>& type, const std::function<void(const Json::Value&, const std::string&)>& callback) {
    auto dbClient = drogon::app().getDbClient();

    // Calcul de la tolérance de simplification selon le zoom
    double tolerance;
    int limit;
    if (zoom <= 6) {
        tolerance = 0.05;  // Simplification agressive à bas zoom
        limit = 2000;
    } else if (zoom <= 10) {
        tolerance = 0.01;
        limit = 5000;
    } else if (zoom <= 14) {
        tolerance = 0.005;
        limit = 10000;
    } else {
        tolerance = 0.001; // Détails à haut zoom
        limit = 20000;
    }

    LOG_INFO << "Obstacles query: zoom=" << zoom << ", tolerance=" << tolerance << ", limit=" << limit;

    // Construct SQL query with simplification and limit
    std::string sql = R"(SELECT jsonb_build_object(
        'type', 'FeatureCollection',
        'features', jsonb_agg(jsonb_build_object(
            'type', 'Feature',
            'geometry', ST_AsGeoJSON(ST_Simplify(t.geom, )" + std::to_string(tolerance) + R"())::jsonb,
            'properties', jsonb_build_object(
                'id', t.id,
                'type', t.type,
                'geom_type', t.geom_type
            )
        ))
    ) AS geojson
    FROM (
        SELECT id, type, geom_type, geom
        FROM obstacle
        WHERE ST_Intersects(geom, ST_MakeEnvelope($1, $2, $3, $4, 4326))
        )" + (type.has_value() ? "AND type = $5 " : "") + R"(
        LIMIT )" + std::to_string(limit) + R"(
    ) t)";

    if (type.has_value()) {
        dbClient->execSqlAsync(
            sql,
            [callback](const drogon::orm::Result& result) {
                        if (!result.empty()) {
                            const auto& row = result[0];
                            std::string geoString = row["geojson"].as<std::string>();
                            Json::CharReaderBuilder builder;
                            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                            Json::Value parsed;
                            std::string errs;
                            if (reader->parse(geoString.c_str(), geoString.c_str() + geoString.length(), &parsed, &errs)) {
                                // Ensure 'features' is always an array (Postgres jsonb_agg returns null when no rows)
                                if (parsed.isObject() && parsed["features"].isNull()) {
                                    parsed["features"] = Json::Value(Json::arrayValue);
                                }
                                callback(parsed, "");
                            } else {
                                LOG_ERROR << "ObstacleService::getByBoundingBox - Failed to parse GeoJSON: " << errs;
                                Json::Value emptyGeoJSON;
                                emptyGeoJSON["type"] = "FeatureCollection";
                                emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                                callback(emptyGeoJSON, "");
                            }
                        } else {
                            Json::Value emptyGeoJSON;
                            emptyGeoJSON["type"] = "FeatureCollection";
                            emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                            callback(emptyGeoJSON, "");
                        }
            },
            [callback](const drogon::orm::DrogonDbException& e) {
                callback(Json::Value(), e.base().what());
            },
            minLon, minLat, maxLon, maxLat, type.value()
        );
    } else {
        dbClient->execSqlAsync(
            sql,
            [callback](const drogon::orm::Result& result) {
                        if (!result.empty()) {
                    const auto& row = result[0];
                    std::string geoString = row["geojson"].as<std::string>();
                    Json::CharReaderBuilder builder;
                    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                    Json::Value parsed;
                    std::string errs;
                    if (reader->parse(geoString.c_str(), geoString.c_str() + geoString.length(), &parsed, &errs)) {
                                // Ensure 'features' is an array (not null) for GeoJSON compliance
                                if (parsed.isObject() && parsed["features"].isNull()) {
                                    parsed["features"] = Json::Value(Json::arrayValue);
                                }
                                callback(parsed, "");
                    } else {
                        LOG_ERROR << "ObstacleService::getByBoundingBox - Failed to parse GeoJSON: " << errs;
                        Json::Value emptyGeoJSON;
                        emptyGeoJSON["type"] = "FeatureCollection";
                        emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                        callback(emptyGeoJSON, "");
                    }
                } else {
                    Json::Value emptyGeoJSON;
                    emptyGeoJSON["type"] = "FeatureCollection";
                    emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                    callback(emptyGeoJSON, "");
                }
            },
            [callback](const drogon::orm::DrogonDbException& e) {
                callback(Json::Value(), e.base().what());
            },
            minLon, minLat, maxLon, maxLat
        );
    }
}
