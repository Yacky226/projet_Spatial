#pragma once
#include <drogon/drogon.h>
#include <string>
#include <vector>

struct SignalReport {
    int antenna_id;
    std::string technology;
    double distance_km;
    double signal_strength_dbm; // Puissance reçue (ex: -90 dBm)
    bool has_obstacle;
    std::string signal_quality; // Excellent, Bon, Moyen, Faible, Nul
    double latitude;
    double longitude;

    Json::Value toJson() const {
        Json::Value ret;
        ret["antenna_id"] = antenna_id;
        ret["technology"] = technology;
        ret["distance_km"] = distance_km;
        ret["signal_dbm"] = signal_strength_dbm;
        ret["has_obstacle"] = has_obstacle;
        ret["quality"] = signal_quality;
        ret["latitude"] = latitude;
        ret["longitude"] = longitude;
        return ret;
    }
};

class SimulationService {
public:
    // Calcule le signal pour un point GPS donné
    static void checkSignalAtPosition(double lat, double lon,
                                      std::optional<int> operatorId,
                                      std::optional<std::string> technology,
                                      std::function<void(const std::vector<SignalReport>&, const std::string&)> callback);
    
private:
    // Helpers internes
    static std::string getQualityLabel(double dbm);
};