#include "SimulationController.h"

void SimulationController::checkSignal(const HttpRequestPtr& req, 
                                       std::function<void (const HttpResponsePtr &)> &&callback,
                                       double lat, double lon) {
    
    // CORRECTION : Ajout de lat et lon dans la capture de la lambda [callback, lat, lon]
    SimulationService::checkSignalAtPosition(lat, lon, 
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