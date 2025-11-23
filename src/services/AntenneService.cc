#include "AntenneService.h"

#include <json/json.h>
#include <optional>

using namespace drogon;
using namespace drogon::orm;

// 1. CREATE
void AntenneService::create(const Antenna &a, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "INSERT INTO antenna (coverage_radius, status, technology, installation_date, operator_id, geom) "
        "VALUES ($1, $2::antenna_status, $3::technology_type, $4, $5, ST_SetSRID(ST_MakePoint($6, $7), 4326))";

    client->execSqlAsync(sql,
                         [callback](const Result &) { callback(""); },
                         [callback](const DrogonDbException &e) { callback(e.base().what()); }, a.coverage_radius,
                         a.status, a.technology,
                         (a.installation_date.empty() ? std::nullopt : std::make_optional(a.installation_date)),
                         a.operator_id, a.longitude, a.latitude);
}

// 2. READ ALL
void AntenneService::getAll(std::function<void(const std::vector<Antenna> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "SELECT id, coverage_radius, status, technology, installation_date::text AS installation_date, "
        "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat FROM antenna ORDER BY id";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<Antenna> list;
            for (auto row : r) {
                Antenna a;
                a.id = row["id"].as<int>();
                a.coverage_radius = row["coverage_radius"].as<double>();
                a.status = row["status"].as<std::string>();
                a.technology = row["technology"].as<std::string>();
                a.installation_date =
                    row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                a.operator_id = row["operator_id"].as<int>();
                a.longitude = row["lon"].as<double>();
                a.latitude = row["lat"].as<double>();
                list.push_back(a);
            }
            callback(list, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); });
}

// 3. READ ONE
void AntenneService::getById(int id, std::function<void(const Antenna &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "SELECT id, coverage_radius, status, technology, installation_date::text AS installation_date, "
        "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat FROM antenna WHERE id = $1";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            if (r.empty()) {
                callback({}, "Not Found");
                return;
            }
            Antenna a;
            auto row = r[0];
            a.id = row["id"].as<int>();
            a.coverage_radius = row["coverage_radius"].as<double>();
            a.status = row["status"].as<std::string>();
            a.technology = row["technology"].as<std::string>();
            a.installation_date =
                row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
            a.operator_id = row["operator_id"].as<int>();
            a.longitude = row["lon"].as<double>();
            a.latitude = row["lat"].as<double>();
            callback(a, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, id);
}

// 4. UPDATE
void AntenneService::update(const Antenna &a, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "UPDATE antenna SET coverage_radius=$1, status=$2::antenna_status, technology=$3::technology_type, "
        "installation_date=$4, operator_id=$5, geom=ST_SetSRID(ST_MakePoint($6, $7), 4326) WHERE id=$8";

    client->execSqlAsync(sql,
                         [callback](const Result &) { callback(""); },
                         [callback](const DrogonDbException &e) { callback(e.base().what()); }, a.coverage_radius,
                         a.status, a.technology,
                         (a.installation_date.empty() ? std::nullopt : std::make_optional(a.installation_date)),
                         a.operator_id, a.longitude, a.latitude, a.id);
}

// 5. DELETE
void AntenneService::remove(int id, std::function<void(const std::string &)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("DELETE FROM antenna WHERE id = $1",
                         [callback](const Result &) { callback(""); },
                         [callback](const DrogonDbException &e) { callback(e.base().what()); }, id);
}

void AntenneService::getInRadius(double lat, double lon, double radiusMeters,
                                 std::function<void(const std::vector<Antenna> &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "SELECT id, coverage_radius, status, technology, installation_date::text AS installation_date, "
        "operator_id, ST_X(geom) AS lon, ST_Y(geom) AS lat "
        "FROM antenna "
        "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3)";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            std::vector<Antenna> list;
            for (auto row : r) {
                Antenna a;
                a.id = row["id"].as<int>();
                a.coverage_radius = row["coverage_radius"].as<double>();
                a.status = row["status"].as<std::string>();
                a.technology = row["technology"].as<std::string>();
                a.installation_date =
                    row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                a.operator_id = row["operator_id"].as<int>();
                a.longitude = row["lon"].as<double>();
                a.latitude = row["lat"].as<double>();
                list.push_back(a);
            }
            callback(list, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, lon, lat, radiusMeters);
}

void AntenneService::getAllGeoJSON(std::function<void(const Json::Value &, const std::string &)> callback) {
    auto client = app().getDbClient();
    std::string sql =
        "SELECT id, coverage_radius, status, technology, installation_date::text AS installation_date, "
        "operator_id, ST_AsGeoJSON(geom) AS geojson FROM antenna";

    client->execSqlAsync(
        sql,
        [callback](const Result &r) {
            Json::Value featureCollection;
            featureCollection["type"] = "FeatureCollection";
            Json::Value features(Json::arrayValue);

            Json::CharReaderBuilder builder;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            std::string errs;

            for (auto row : r) {
                Json::Value feature;
                feature["type"] = "Feature";

                Json::Value props;
                props["id"] = row["id"].as<int>();
                props["technology"] = row["technology"].as<std::string>();
                props["coverage_radius"] = row["coverage_radius"].as<double>();
                props["operator_id"] = row["operator_id"].as<int>();
                props["installation_date"] =
                    row["installation_date"].isNull() ? "" : row["installation_date"].as<std::string>();
                feature["properties"] = props;

                std::string geojsonString = row["geojson"].as<std::string>();
                Json::Value geometryObject;
                if (reader->parse(geojsonString.c_str(), geojsonString.c_str() + geojsonString.length(),
                                  &geometryObject, &errs)) {
                    feature["geometry"] = geometryObject;
                }

                features.append(feature);
            }

            featureCollection["features"] = features;
            callback(featureCollection, "");
        },
        [callback](const DrogonDbException &e) {
            Json::Value empty;
            callback(empty, e.base().what());
        });
}