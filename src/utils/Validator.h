#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <string>
#include <vector>
#include <regex>
#include <optional>

class Validator {
public:
    struct ValidationError {
        std::string field;
        std::string message;
    };

    // Validation des coordonnées GPS
    static bool isValidLatitude(double lat) {
        return lat >= -90.0 && lat <= 90.0;
    }

    static bool isValidLongitude(double lon) {
        return lon >= -180.0 && lon <= 180.0;
    }

    // Validation des nombres positifs
    static bool isPositive(double value) {
        return value > 0;
    }

    static bool isNonNegative(int value) {
        return value >= 0;
    }

    // Validation des chaînes
    static bool isNotEmpty(const std::string& str) {
        return !str.empty() && str.find_first_not_of(" \t\n\r") != std::string::npos;
    }

    static bool hasMaxLength(const std::string& str, size_t maxLen) {
        return str.length() <= maxLen;
    }

    // Validation des énumérations
    static bool isValidStatus(const std::string& status) {
        static const std::vector<std::string> validStatuses = {
            "active", "inactive", "maintenance", "planned"
        };
        return std::find(validStatuses.begin(), validStatuses.end(), status) 
               != validStatuses.end();
    }

    static bool isValidTechnology(const std::string& tech) {
        static const std::vector<std::string> validTechs = {
            "2G", "3G", "4G", "5G"
        };
        return std::find(validTechs.begin(), validTechs.end(), tech) 
               != validTechs.end();
    }

    static bool isValidObstacleType(const std::string& type) {
        return type == "batiment" || type == "vegetation" || type == "relief";
    }

    // Validation d'email
    static bool isValidEmail(const std::string& email) {
        static const std::regex emailPattern(
            R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
        );
        return std::regex_match(email, emailPattern);
    }

    // Validation de téléphone (format international)
    static bool isValidPhone(const std::string& phone) {
        static const std::regex phonePattern(R"(^\+?[1-9]\d{1,14}$)");
        return std::regex_match(phone, phonePattern);
    }

    // Collecteur d'erreurs
    class ErrorCollector {
    private:
        std::vector<ValidationError> errors;

    public:
        void addError(const std::string& field, const std::string& message) {
            errors.push_back({field, message});
        }

        bool hasErrors() const {
            return !errors.empty();
        }

        std::string getErrorsAsJson() const {
            Json::Value root;
            root["success"] = false;
            Json::Value errorsArray(Json::arrayValue);
            
            for (const auto& error : errors) {
                Json::Value errorObj;
                errorObj["field"] = error.field;
                errorObj["message"] = error.message;
                errorsArray.append(errorObj);
            }
            
            root["errors"] = errorsArray;
            
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            return Json::writeString(builder, root);
        }

        const std::vector<ValidationError>& getErrors() const {
            return errors;
        }
    };
};

#endif