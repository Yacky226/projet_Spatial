#include "AntenneService.h"

using namespace drogon;
using namespace drogon::orm;
#include <json/json.h> 

// 1. CREATE
void AntenneService::create(const Antenna &a, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    // Insertion avec conversion Lat/Lon vers Geometry
    std::string sql = "INSERT INTO antenna (coverage_radius, status, technology, operator_id, geom) "
                      "VALUES ($1, $2::antenna_status, $3::technology_type, $4, ST_SetSRID(ST_MakePoint($5, $6), 4326))";

    client->execSqlAsync(sql, 
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        a.coverage_radius, a.status, a.technology, a.operator_id, a.longitude, a.latitude
    );
}

// 2. READ ALL
void AntenneService::getAll(std::function<void(const std::vector<Antenna>&, const std::string&)> callback) {
    auto client = app().getDbClient();
    // Extraction Lat/Lon depuis Geometry
    std::string sql = "SELECT id, coverage_radius, status, technology, operator_id, ST_X(geom) as lon, ST_Y(geom) as lat FROM antenna ORDER BY id";

    client->execSqlAsync(sql, [callback](const Result &r) {
        std::vector<Antenna> list;
        for (auto row : r) {
            Antenna a;
            a.id = row["id"].as<int>();
            a.coverage_radius = row["coverage_radius"].as<double>();
            a.status = row["status"].as<std::string>();
            a.technology = row["technology"].as<std::string>();
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
void AntenneService::getById(int id, std::function<void(const Antenna&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, coverage_radius, status, technology, operator_id, ST_X(geom) as lon, ST_Y(geom) as lat FROM antenna WHERE id = $1";

    client->execSqlAsync(sql, [callback](const Result &r) {
        if (r.size() == 0) { callback({}, "Not Found"); return; }
        Antenna a;
        auto row = r[0];
        a.id = row["id"].as<int>();
        a.coverage_radius = row["coverage_radius"].as<double>();
        a.status = row["status"].as<std::string>();
        a.technology = row["technology"].as<std::string>();
        a.operator_id = row["operator_id"].as<int>();
        a.longitude = row["lon"].as<double>();
        a.latitude = row["lat"].as<double>();
        callback(a, "");
    },
    [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, id);
}

// 4. UPDATE
void AntenneService::update(const Antenna &a, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    // Mise à jour complète, y compris la position géographique
    std::string sql = "UPDATE antenna SET coverage_radius=$1, status=$2::antenna_status, "
                      "technology=$3::technology_type, operator_id=$4, "
                      "geom=ST_SetSRID(ST_MakePoint($5, $6), 4326) WHERE id=$7";

    client->execSqlAsync(sql, 
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        a.coverage_radius, a.status, a.technology, a.operator_id, a.longitude, a.latitude, a.id
    );
}

// 5. DELETE
void AntenneService::remove(int id, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("DELETE FROM antenna WHERE id = $1",
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        id
    );
}

void AntenneService::getInRadius(double lat, double lon, double radiusMeters, 
                                 std::function<void(const std::vector<Antenna>&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // REQUÊTE POSTGIS CLÉ :
    // 1. ST_MakePoint($1, $2) : Crée le point central (le client).
    // 2. ST_SetSRID(..., 4326) : Dit que c'est du GPS (WGS84).
    // 3. ::geography : Convertit en "géographie" pour que le rayon soit en MÈTRES (et pas en degrés).
    // 4. ST_DWithin(...) : Trouve tout ce qui est dans le rayon.
    
    std::string sql = "SELECT id, coverage_radius, status, technology, operator_id, "
                      "ST_X(geom) as lon, ST_Y(geom) as lat "
                      "FROM antenna "
                      "WHERE ST_DWithin(geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, $3)";

    client->execSqlAsync(sql, 
        [callback](const Result &r) {
            std::vector<Antenna> list;
            for (auto row : r) {
                Antenna a;
                a.id = row["id"].as<int>();
                a.coverage_radius = row["coverage_radius"].as<double>();
                a.status = row["status"].as<std::string>();
                a.technology = row["technology"].as<std::string>();
                a.operator_id = row["operator_id"].as<int>();
                a.longitude = row["lon"].as<double>();
                a.latitude = row["lat"].as<double>();
                list.push_back(a);
            }
            callback(list, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); },
        lon, lat, radiusMeters // Attention à l'ordre : PostGIS prend (Longitude, Latitude)
    );
}



void AntenneService::getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // SQL : On demande directement le GeoJSON à la base de données
    std::string sql = "SELECT id, coverage_radius, status, technology, operator_id, "
                      "ST_AsGeoJSON(geom) as geojson " // <--- La magie est ici
                      "FROM antenna";

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

            // 1. Propriétés (Données métier)
            Json::Value props;
            props["id"] = row["id"].as<int>();
            props["technology"] = row["technology"].as<std::string>();
            props["coverage_radius"] = row["coverage_radius"].as<double>();
            props["operator_id"] = row["operator_id"].as<int>();
            feature["properties"] = props;

            // 2. Géométrie (Parsing de la string renvoyée par PostGIS)
            std::string geojsonString = row["geojson"].as<std::string>();
            Json::Value geometryObject;
            
            // On transforme la string "{...}" en Objet JSON réel
            if (reader->parse(geojsonString.c_str(), geojsonString.c_str() + geojsonString.length(), &geometryObject, &errs)) {
                feature["geometry"] = geometryObject;
            }

            features.append(feature);
        }

        featureCollection["features"] = features;
        callback(featureCollection, ""); // On renvoie directement un Json::Value
    },
    [callback](const DrogonDbException &e) { 
        Json::Value empty;
        callback(empty, e.base().what()); 
    });
}