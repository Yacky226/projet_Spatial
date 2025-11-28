#include "SimulationService.h"
#include <cmath>

using namespace drogon;
using namespace drogon::orm;

// Fréquences standards (simplifiées pour la simulation)
const double FREQ_4G = 2600.0; // MHz
const double FREQ_5G = 3500.0; // MHz

// Puissance d'émission standard (EIRP) en dBm
const double POWER_4G = 46.0; // ~40 Watts
const double POWER_5G = 50.0; // ~100 Watts

// Perte par obstacle (béton/brique moyen)
const double OBSTACLE_LOSS = 25.0; // dB

void SimulationService::checkSignalAtPosition(double lat, double lon, 
                                              std::function<void(const std::vector<SignalReport>&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // REQUÊTE MAGIQUE :
    // 1. Trouve les antennes proches (< 5km)
    // 2. Calcule la distance exacte
    // 3. Vérifie s'il y a un obstacle qui coupe la ligne (ST_Intersects avec une Ligne)
    std::string sql = R"(
        SELECT 
            a.id, 
            a.technology,
            ST_Distance(a.geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography) / 1000.0 as dist_km,
            EXISTS(
                SELECT 1 FROM obstacle o 
                WHERE ST_Intersects(
                    ST_MakeLine(a.geom::geometry, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geometry), 
                    o.geom::geometry
                )
            ) as blocked
        FROM antenna a
        WHERE ST_DWithin(a.geom::geography, ST_SetSRID(ST_MakePoint($1, $2), 4326)::geography, 5000)
    )";

    client->execSqlAsync(sql, 
        [callback](const Result &r) {
            std::vector<SignalReport> reports;
            for (auto row : r) {
                SignalReport report;
                report.antenna_id = row["id"].as<int>();
                report.technology = row["technology"].as<std::string>();
                report.distance_km = row["dist_km"].as<double>();
                report.has_obstacle = row["blocked"].as<bool>();

                // 1. Choix des paramètres selon la techno
                double freq = (report.technology == "5G") ? FREQ_5G : FREQ_4G;
                double tx_power = (report.technology == "5G") ? POWER_5G : POWER_4G;

                // 2. Calcul de la perte en espace libre (FSPL)
                // Formule : 20*log10(d) + 20*log10(f) + 32.45
                double fspl = 20 * log10(report.distance_km) + 20 * log10(freq) + 32.45;

                // 3. Bilan de liaison
                double rx_power = tx_power - fspl;

                // 4. Pénalité obstacle
                if (report.has_obstacle) {
                    rx_power -= OBSTACLE_LOSS;
                }

                report.signal_strength_dbm = round(rx_power * 100) / 100; // Arrondi
                report.signal_quality = getQualityLabel(report.signal_strength_dbm);

                // On ne garde que si le signal est captable (> -120 dBm)
                if (report.signal_strength_dbm > -120.0) {
                    reports.push_back(report);
                }
            }
            callback(reports, "");
        },
        [callback](const DrogonDbException &e) { callback({}, e.base().what()); },
        lon, lat
    );
}

std::string SimulationService::getQualityLabel(double dbm) {
    if (dbm >= -80) return "Excellent"; // 5 barres
    if (dbm >= -95) return "Bon";       // 4 barres
    if (dbm >= -105) return "Moyen";    // 3 barres
    if (dbm >= -115) return "Faible";   // 1-2 barres
    return "Nul";                       // Pas de service
}