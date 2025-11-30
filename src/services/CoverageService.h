#pragma once
#include "../models/CoverageResult.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <functional>
#include <optional>

class CoverageService {
public:
    // Calcul de couverture pour une antenne
    static void calculateForAntenna(
        int antennaId, 
        std::function<void(const CoverageResult&, const std::string&)> callback
    );
    
    // Zones blanches (version simple - tous opérateurs)
    static void getWhiteZones(
        int zone_id,
        std::function<void(const Json::Value&, const std::string&)> callback
    );
    
    //  Zones blanches avancées (avec filtre opérateur optionnel)
    static void getWhiteZonesAdvanced(
        int zone_id,
        std::optional<int> operator_id,  // std::nullopt = tous opérateurs
        std::function<void(const Json::Value&, const std::string&)> callback
    );

private:
    // Fonction helper pour traiter le résultat JSON
    static void handleWhiteZonesResult(
        const drogon::orm::Result& r,
        std::function<void(const Json::Value&, const std::string&)> callback
    );
};