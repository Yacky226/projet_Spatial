#include "ObstacleService.h"

using namespace drogon;
using namespace drogon::orm;

// 1. CREATE
void ObstacleService::create(const ObstacleModel &obs, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    // Insertion : On stocke le type de géométrie et la géométrie elle-même via ST_GeomFromText
    std::string sql = "INSERT INTO obstacle (type, geom_type, geom) "
                      "VALUES ($1, $2, ST_GeomFromText($3, 4326))";

    client->execSqlAsync(sql, 
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        obs.type, obs.geom_type, obs.wkt_geometry
    );
}

// 2. READ ALL
void ObstacleService::getAll(std::function<void(const std::vector<ObstacleModel>&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, type, geom_type, ST_AsText(geom) as wkt FROM obstacle ORDER BY id";

    client->execSqlAsync(sql, [callback](const Result &r) {
        std::vector<ObstacleModel> list;
        for (auto row : r) {
            ObstacleModel o;
            o.id = row["id"].as<int>();
            o.type = row["type"].as<std::string>();
            o.geom_type = row["geom_type"].as<std::string>();
            o.wkt_geometry = row["wkt"].as<std::string>();
            list.push_back(o);
        }
        callback(list, "");
    },
    [callback](const DrogonDbException &e) { callback({}, e.base().what()); });
}

// 3. READ ONE
void ObstacleService::getById(int id, std::function<void(const ObstacleModel&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, type, geom_type, ST_AsText(geom) as wkt FROM obstacle WHERE id = $1";

    client->execSqlAsync(sql, [callback](const Result &r) {
        if (r.size() == 0) { callback({}, "Not Found"); return; }
        auto row = r[0];
        ObstacleModel o;
        o.id = row["id"].as<int>();
        o.type = row["type"].as<std::string>();
        o.geom_type = row["geom_type"].as<std::string>();
        o.wkt_geometry = row["wkt"].as<std::string>();
        callback(o, "");
    },
    [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, id);
}

// 4. UPDATE
void ObstacleService::update(const ObstacleModel &obs, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "UPDATE obstacle SET type=$1, geom_type=$2, geom=ST_GeomFromText($3, 4326) WHERE id=$4";

    client->execSqlAsync(sql, 
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        obs.type, obs.geom_type, obs.wkt_geometry, obs.id
    );
}

// 5. DELETE
void ObstacleService::remove(int id, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("DELETE FROM obstacle WHERE id = $1",
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        id
    );

}

 void ObstacleService::getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // On récupère le type d'obstacle et sa géométrie au format GeoJSON
    std::string sql = "SELECT id, type, geom_type, ST_AsGeoJSON(geom) as geojson FROM obstacle";

    client->execSqlAsync(sql, [callback](const Result &r) {
        Json::Value featureCollection;
        featureCollection["type"] = "FeatureCollection";
        Json::Value features(Json::arrayValue);

        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        std::string errs;

        for (auto row : r) {
            Json::Value feature;
            feature["type"] = "Feature";

            // Propriétés (Type de bâtiment, hauteur, etc.)
            Json::Value props;
            props["id"] = row["id"].as<int>();
            props["type"] = row["type"].as<std::string>();         // ex: 'batiment'
            props["geom_type"] = row["geom_type"].as<std::string>(); // ex: 'POLYGON'
            feature["properties"] = props;

            // Parsing de la géométrie GeoJSON brute
            std::string geoString = row["geojson"].as<std::string>();
            Json::Value geomObj;
            if(reader->parse(geoString.c_str(), geoString.c_str() + geoString.length(), &geomObj, &errs)){
                feature["geometry"] = geomObj;
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