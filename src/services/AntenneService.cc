#include "AntenneService.h"
#include <drogon/orm/DbClient.h> // Inclusion explicite pour éviter les ambiguïtés

using namespace drogon;
using namespace drogon::orm; // Permet d'utiliser Result et DrogonDbException directement

void AntenneService::createAntenne(const AntenneModel &antenne, 
                                   std::function<void(const std::string& error)> callback) {
    auto client = app().getDbClient();

    // Requete SQL
    std::string sql = "INSERT INTO antenna (technology, coverage_radius, operator_id, geom) "
                      "VALUES ($1, $2, $3, ST_SetSRID(ST_MakePoint($4, $5), 4326))";

    // Notez l'utilisation de 'Result' et 'DrogonDbException' sans 'drm::'
    client->execSqlAsync(sql, 
        [callback](const Result &res) {
            callback(""); // Succès
        },
        [callback](const DrogonDbException &e) {
            LOG_ERROR << "DB Error: " << e.base().what();
            callback(e.base().what());
        },
        antenne.technology, 
        antenne.coverage_radius, 
        antenne.operator_id, 
        antenne.longitude, 
        antenne.latitude
    );
}

void AntenneService::getAllAntennes(std::function<void(const std::vector<AntenneModel>&, const std::string&)> callback) {
    auto client = app().getDbClient();
    
    std::string sql = "SELECT id, technology, coverage_radius, operator_id, "
                      "ST_X(geom) as lon, ST_Y(geom) as lat FROM antenna";

    client->execSqlAsync(sql, [callback](const Result &res) {
        std::vector<AntenneModel> result;
        for (auto row : res) {
            AntenneModel a;
            a.id = row["id"].as<int>();
            a.technology = row["technology"].as<std::string>();
            a.coverage_radius = row["coverage_radius"].as<double>();
            a.operator_id = row["operator_id"].as<int>();
            a.longitude = row["lon"].as<double>();
            a.latitude = row["lat"].as<double>();
            result.push_back(a);
        }
        callback(result, "");
    },
    [callback](const DrogonDbException &e) {
        LOG_ERROR << "DB Error: " << e.base().what();
        callback({}, e.base().what());
    });
}