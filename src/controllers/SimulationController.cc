#include "SimulationController.h"

// Vérification du signal radio à une position donnée
void SimulationController::checkSignal(const HttpRequestPtr& req,
                                       std::function<void (const HttpResponsePtr &)> &&callback) {

    // Récupération des paramètres
    auto& params = req->getParameters();

    // Vérification des params obligatoires
    if (params.find("lat") == params.end() || params.find("lon") == params.end()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing required parameters: lat and lon");
        callback(resp);
        return;
    }

    // Conversion des coordonnées
    double lat = std::stod(params.at("lat"));
    double lon = std::stod(params.at("lon"));

    // Params optionnels
    std::optional<int> operatorId = std::nullopt;
    if (params.find("operatorId") != params.end() && !params.at("operatorId").empty()) {
        operatorId = std::stoi(params.at("operatorId"));
    }

    std::optional<std::string> technology = std::nullopt;
    if (params.find("technology") != params.end() && !params.at("technology").empty()) {
        technology = params.at("technology");
    }

    // Appel du service
    SimulationService::checkSignalAtPosition(lat, lon, operatorId, technology,
        [callback, lat, lon](const std::vector<SignalReport>& reports, const std::string& err) {
            if (err.empty()) {
                // Tri par puissance décroissante
                auto sortedReports = reports;
                std::sort(sortedReports.begin(), sortedReports.end(), 
                    [](const SignalReport& a, const SignalReport& b) {
                        return a.signal_strength_dbm > b.signal_strength_dbm;
                    });

                // Construction de la réponse JSON
                Json::Value jsonArr(Json::arrayValue);
                for(const auto& r : sortedReports) {
                    jsonArr.append(r.toJson());
                }

                Json::Value result;
                result["location"]["lat"] = lat;
                result["location"]["lon"] = lon;
                result["antennas_visible"] = (int)sortedReports.size();
                result["network_quality"] = sortedReports.empty() ? "Aucun Service" : sortedReports[0].signal_quality;
                result["details"] = jsonArr;

                auto resp = HttpResponse::newHttpJsonResponse(result);
                callback(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody(err);
                callback(resp);
            }
        }
    );
}