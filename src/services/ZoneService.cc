#include "ZoneService.h"
using namespace drogon;
using namespace drogon::orm;

// 1. CREATE
void ZoneService::create(const ZoneModel &z, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    
    // Gestion du parent_id (NULL si 0)
    std::string sql = "INSERT INTO zone (name, type, density, geom, parent_id) "
                      "VALUES ($1, $2::zone_type, $3, ST_GeomFromText($4, 4326), $5)";

    // Astuce : Si parent_id est 0, on envoie nullptr pour que SQL mette NULL
    // Mais Drogon gère mal nullptr direct, on envoie NULL via conditionnel SQL ou on laisse 0 si la FK l'autorise. 
    // Ici on suppose que la DB accepte NULL. Pour simplifier en C++ sans std::optional, on envoie z.parent_id si > 0, sinon NULL est géré par la logique SQL.
    // Pour faire simple ici : on utilise une requête dynamique ou on accepte NULL.
    // Note: Pour simplifier ce code exemple, on insère NULL si parent_id == 0 via SQL CASE ou similaire est complexe.
    // On va assumer que l'utilisateur ne met pas de parent_id pour l'instant ou envoie un ID valide.
    
    client->execSqlAsync(sql, 
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        z.name, z.type, z.density, z.wkt_geometry, (z.parent_id > 0 ? std::make_optional(z.parent_id) : std::nullopt)
    );
}

// 2. READ ALL
void ZoneService::getAll(std::function<void(const std::vector<ZoneModel>&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, name, type, density, parent_id, ST_AsText(geom) as wkt FROM zone ORDER BY id";

    client->execSqlAsync(sql, [callback](const Result &r) {
        std::vector<ZoneModel> list;
        for (auto row : r) {
            ZoneModel z;
            z.id = row["id"].as<int>();
            z.name = row["name"].as<std::string>();
            z.type = row["type"].as<std::string>();
            z.density = row["density"].as<double>();
            z.parent_id = row["parent_id"].isNull() ? 0 : row["parent_id"].as<int>();
            z.wkt_geometry = row["wkt"].as<std::string>();
            list.push_back(z);
        }
        callback(list, "");
    }, [callback](const DrogonDbException &e) { callback({}, e.base().what()); });
}

// 3. READ ONE
void ZoneService::getById(int id, std::function<void(const ZoneModel&, const std::string&)> callback) {
    auto client = app().getDbClient();
    std::string sql = "SELECT id, name, type, density, parent_id, ST_AsText(geom) as wkt FROM zone WHERE id = $1";

    client->execSqlAsync(sql, [callback](const Result &r) {
        if (r.size() == 0) { callback({}, "Not Found"); return; }
        auto row = r[0];
        ZoneModel z;
        z.id = row["id"].as<int>();
        z.name = row["name"].as<std::string>();
        z.type = row["type"].as<std::string>();
        z.density = row["density"].as<double>();
        z.parent_id = row["parent_id"].isNull() ? 0 : row["parent_id"].as<int>();
        z.wkt_geometry = row["wkt"].as<std::string>();
        callback(z, "");
    }, [callback](const DrogonDbException &e) { callback({}, e.base().what()); }, id);
}

// 4. UPDATE
void ZoneService::update(const ZoneModel &z, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    // Update complet avec géométrie
    std::string sql = "UPDATE zone SET name=$1, type=$2::zone_type, density=$3, "
                      "geom=ST_GeomFromText($4, 4326), parent_id=$5 WHERE id=$6";

    client->execSqlAsync(sql, 
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        z.name, z.type, z.density, z.wkt_geometry, (z.parent_id > 0 ? std::make_optional(z.parent_id) : std::nullopt), z.id
    );
}

// 5. DELETE
void ZoneService::remove(int id, std::function<void(const std::string&)> callback) {
    auto client = app().getDbClient();
    client->execSqlAsync("DELETE FROM zone WHERE id = $1",
        [callback](const Result &r) { callback(""); },
        [callback](const DrogonDbException &e) { callback(e.base().what()); },
        id);
}

void ZoneService::getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback) {
    auto client = app().getDbClient();
    
    // ST_AsGeoJSON fonctionne aussi parfaitement sur des POLYGONES
    std::string sql = "SELECT id, name, type, density, ST_AsGeoJSON(geom) as geojson FROM zone";

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

            // Propriétés
            Json::Value props;
            props["id"] = row["id"].as<int>();
            props["name"] = row["name"].as<std::string>();
            props["density"] = row["density"].as<double>();
            feature["properties"] = props;

            // Géométrie
            std::string geoStr = row["geojson"].as<std::string>();
            Json::Value geomObj;
            if(reader->parse(geoStr.c_str(), geoStr.c_str() + geoStr.length(), &geomObj, &errs)){
                feature["geometry"] = geomObj;
            }
            features.append(feature);
        }
        featureCollection["features"] = features;
        callback(featureCollection, "");
    },
    [callback](const DrogonDbException &e) { 
        Json::Value empty; callback(empty, e.base().what()); 
    });
}