#include "SimulationService.h"
#include <cmath>

using namespace drogon;
using namespace drogon::orm;

// Fréquences standards utilisées pour les calculs de simulation
const double FREQ_4G = 2600.0; // MHz
const double FREQ_5G = 3500.0; // MHz

// Puissance d'émission effective (EIRP) en dBm pour chaque technologie
const double POWER_4G = 46.0; // ~40 Watts
const double POWER_5G = 50.0; // ~100 Watts

// Atténuation moyenne causée par les obstacles en béton/brique
const double OBSTACLE_LOSS = 25.0; // dB

void SimulationService::checkSignalAtPosition(double lat, double lon,
                                              std::optional<int> operatorId,
                                              std::optional<std::string> technology,
                                              std::function<void(const std::vector<SignalReport>&, const std::string&)> callback) {
    auto client = app().getDbClient();

    // Requête complexe pour analyser la couverture radio :
    // - Recherche des antennes dans un rayon de 5km
    // - Calcul précis des distances
    // - Détection des obstacles sur la ligne de vue
    // - Extraction des coordonnées des antennes
    // - Filtrage optionnel par opérateur/technologie
    std::string sql = R"(
        SELECT 
            a.id, 
            a.technology,
            ST_X(a.geom::geometry) as longitude,
            ST_Y(a.geom::geometry) as latitude,
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

    // Ajouter les filtres selon les paramètres
    if (operatorId.has_value()) {
        sql += " AND a.operator_id = " + std::to_string(operatorId.value());
    }
    if (technology.has_value() && !technology.value().empty()) {
        sql += " AND a.technology = '" + technology.value() + "'";
    }

    client->execSqlAsync(sql, 
        [callback](const Result &r) {
            std::vector<SignalReport> reports;
            for (auto row : r) {
                SignalReport report;
                report.antenna_id = row["id"].as<int>();
                report.technology = row["technology"].as<std::string>();
                report.latitude = row["latitude"].as<double>();
                report.longitude = row["longitude"].as<double>();
                report.distance_km = row["dist_km"].as<double>();
                report.has_obstacle = row["blocked"].as<bool>();

                // Sélection des paramètres radio selon la technologie
                double freq = (report.technology == "5G") ? FREQ_5G : FREQ_4G;
                double tx_power = (report.technology == "5G") ? POWER_5G : POWER_4G;

                // Calcul de la perte en espace libre selon la formule standard
                // FSPL = 20*log10(distance) + 20*log10(fréquence) + 32.45
                double fspl = 20 * log10(report.distance_km) + 20 * log10(freq) + 32.45;

                // Calcul de la puissance reçue avant pénalités
                double rx_power = tx_power - fspl;

                // Application de la pénalité si un obstacle bloque la ligne de vue
                if (report.has_obstacle) {
                    rx_power -= OBSTACLE_LOSS;
                }

                report.signal_strength_dbm = round(rx_power * 100) / 100; // Arrondi à 2 décimales
                report.signal_quality = getQualityLabel(report.signal_strength_dbm);

                // Filtrage des signaux trop faibles (< -120 dBm = seuil de détection)
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
    // Classification de la qualité du signal selon les seuils standard
    if (dbm >= -80) return "Excellent"; // Signal très fort
    if (dbm >= -95) return "Bon";       // Signal correct
    if (dbm >= -105) return "Moyen";    // Signal acceptable
    if (dbm >= -115) return "Faible";   // Signal faible mais utilisable
    return "Nul";                       // Pas de service
}