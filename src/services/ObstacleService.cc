#include "ObstacleService.h"
#include "../utils/ErrorHandler.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <memory>
#include <optional>

void ObstacleService::getByBoundingBox(double minLon, double minLat, double maxLon, double maxLat, int zoom, const std::optional<std::string>& type, const std::function<void(const Json::Value&, const std::string&)>& callback) {
    auto dbClient = drogon::app().getDbClient();

    // Ajustement de la tolérance de simplification selon le niveau de zoom pour optimiser les performances
    double tolerance;
    int limit;
    if (zoom <= 6) {
        tolerance = 0.05;  // Simplification forte à bas zoom pour réduire la charge
        limit = 2000;
    } else if (zoom <= 10) {
        tolerance = 0.01;
        limit = 5000;
    } else if (zoom <= 14) {
        tolerance = 0.005;
        limit = 10000;
    } else {
        tolerance = 0.001; // Détails préservés à haut zoom
        limit = 20000;
    }

    LOG_INFO << "Obstacles query: zoom=" << zoom << ", tolerance=" << tolerance << ", limit=" << limit;

    // Construction de la requête SQL avec simplification géométrique et limitation du nombre de résultats
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
                                // S'assurer que 'features' est toujours un tableau (jsonb_agg retourne null si pas de résultats)
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
                            // Retourner une collection GeoJSON vide si aucun résultat
                            Json::Value emptyGeoJSON;
                            emptyGeoJSON["type"] = "FeatureCollection";
                            emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                            callback(emptyGeoJSON, "");
                        }
            },
            [callback](const drogon::orm::DrogonDbException& e) {
                // Gestion des erreurs de base de données
                callback(Json::Value(), e.base().what());
            },
            minLon, minLat, maxLon, maxLat, type.value()
        );
    } else {
        // Requête sans filtrage par type
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
                                // S'assurer que 'features' est un tableau pour la conformité GeoJSON
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
                    // Collection GeoJSON vide si aucun obstacle trouvé
                    Json::Value emptyGeoJSON;
                    emptyGeoJSON["type"] = "FeatureCollection";
                    emptyGeoJSON["features"] = Json::Value(Json::arrayValue);
                    callback(emptyGeoJSON, "");
                }
            },
            [callback](const drogon::orm::DrogonDbException& e) {
                // Gestion des erreurs de base de données
                callback(Json::Value(), e.base().what());
            },
            minLon, minLat, maxLon, maxLat
        );
    }
}
