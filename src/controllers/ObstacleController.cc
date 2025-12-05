#include "ObstacleController.h"
#include "../utils/Validator.h"
#include "../utils/ErrorHandler.h"
#include <drogon/HttpResponse.h>
#include <sstream>

using namespace drogon;

// ============================================================================
// GET OBSTACLES BY BOUNDING BOX
// ============================================================================
void ObstacleController::getByBoundingBox(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
    auto bbox = req->getParameter("bbox"); // Format: minLon,minLat,maxLon,maxLat
    auto type = req->getOptionalParameter<std::string>("type"); // Optional filter by type
    int zoom = req->getOptionalParameter<int>("zoom").value_or(10); // Default zoom 10

    // Parse bbox into coordinates (robust)
    std::vector<std::string> coords;
    std::stringstream ss(bbox);
    std::string item;
    while (std::getline(ss, item, ',')) {
        coords.push_back(item);
    }
    if (coords.size() != 4) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid bounding box format. Expected: minLon,minLat,maxLon,maxLat");
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        callback(resp);
        return;
    }

    auto trim = [](const std::string &s) {
        const char* ws = " \t\n\r";
        size_t b = s.find_first_not_of(ws);
        if (b == std::string::npos) return std::string();
        size_t e = s.find_last_not_of(ws);
        return s.substr(b, e - b + 1);
    };

    double minLon, minLat, maxLon, maxLat;
    try {
        minLon = std::stod(trim(coords[0]));
        minLat = std::stod(trim(coords[1]));
        maxLon = std::stod(trim(coords[2]));
        maxLat = std::stod(trim(coords[3]));
    } catch (const std::invalid_argument &e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Bounding box contains invalid number");
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        callback(resp);
        return;
    } catch (const std::out_of_range &e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Bounding box number out of range");
        resp->setContentTypeCode(CT_TEXT_PLAIN);
        callback(resp);
        return;
    }

    Validator::ErrorCollector validatorCoords;
    if (!Validator::isValidLongitude(minLon) || !Validator::isValidLongitude(maxLon)) {
        validatorCoords.addError("longitude", "Longitudes must be between -180 and +180 degrees");
    }
    if (!Validator::isValidLatitude(minLat) || !Validator::isValidLatitude(maxLat)) {
        validatorCoords.addError("latitude", "Latitudes must be between -90 and +90 degrees");
    }
    if (minLat >= maxLat) {
        validatorCoords.addError("bbox", "minLat must be less than maxLat");
    }
    if (minLon >= maxLon) {
        validatorCoords.addError("bbox", "minLon must be less than maxLon");
    }
    if (validatorCoords.hasErrors()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody(validatorCoords.getErrorsAsJson());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }

    // Query database
    ObstacleService::getByBoundingBox(minLon, minLat, maxLon, maxLat, zoom, type, [callback](const Json::Value& geojson, const std::string& err) {
        if (err.empty()) {
            auto resp = HttpResponse::newHttpJsonResponse(geojson);
            resp->addHeader("Content-Type", "application/geo+json");
            callback(resp);
        } else {
            auto errorDetails = ErrorHandler::analyzePostgresError(err);
            ErrorHandler::logError("ObstacleController::getByBoundingBox", errorDetails);
            auto resp = ErrorHandler::createErrorResponse(errorDetails);
            callback(resp);
        }
    });
}