#include "SimulationController.h"

void SimulationController::checkSignal(const HttpRequestPtr& req,
                                       std::function<void (const HttpResponsePtr &)> &&callback) {

    // Extraire les paramètres de la requête
    auto& params = req->getParameters();

    // Vérifier que les paramètres requis sont présents
    if (params.find("lat") == params.end() || params.find("lon") == params.end()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing required parameters: lat and lon");
        callback(resp);
        return;
    }

    double lat = std::stod(params.at("lat"));
    double lon = std::stod(params.at("lon"));

    std::optional<int> operatorId = std::nullopt;
    if (params.find("operatorId") != params.end() && !params.at("operatorId").empty()) {
        operatorId = std::stoi(params.at("operatorId"));
    }

    std::optional<std::string> technology = std::nullopt;
    if (params.find("technology") != params.end() && !params.at("technology").empty()) {
        technology = params.at("technology");
    }

    // CORRECTION : Ajout de lat et lon dans la capture de la lambda [callback, lat, lon]
    SimulationService::checkSignalAtPosition(lat, lon, operatorId, technology,
        [callback, lat, lon](const std::vector<SignalReport>& reports, const std::string& err) {
            if (err.empty()) {
                // On trie pour avoir la meilleure antenne en premier
                auto sortedReports = reports;
                std::sort(sortedReports.begin(), sortedReports.end(), 
                    [](const SignalReport& a, const SignalReport& b) {
                        return a.signal_strength_dbm > b.signal_strength_dbm;
                    });

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