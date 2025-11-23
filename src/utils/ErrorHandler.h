#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <json/json.h>
#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>

using namespace drogon;

class ErrorHandler {
public:
    // Types d'erreurs
    enum class ErrorType {
        DATABASE_ERROR,
        FOREIGN_KEY_VIOLATION,
        UNIQUE_VIOLATION,
        NOT_FOUND,
        VALIDATION_ERROR,
        UNKNOWN_ERROR
    };

    // Structure d'erreur détaillée
    struct ErrorDetails {
        ErrorType type;
        std::string userMessage;      // Message pour l'utilisateur
        std::string technicalMessage; // Message technique pour les logs
        HttpStatusCode statusCode;
        std::string errorCode;        // Code d'erreur unique (ex: "ERR_ANT_001")
    };

    // Analyser une erreur PostgreSQL
    static ErrorDetails analyzePostgresError(const std::string& pgError) {
        ErrorDetails details;
        details.technicalMessage = pgError;

        // Détection de violation de clé étrangère
        if (pgError.find("foreign key constraint") != std::string::npos ||
            pgError.find("violates foreign key") != std::string::npos) {
            
            details.type = ErrorType::FOREIGN_KEY_VIOLATION;
            details.statusCode = k400BadRequest;
            details.errorCode = "ERR_DB_FK_001";

            // Extraction du nom de la contrainte
            if (pgError.find("operator_id") != std::string::npos) {
                details.userMessage = "The specified operator does not exist. Please provide a valid operator_id.";
            } else if (pgError.find("zone_id") != std::string::npos) {
                details.userMessage = "The specified zone does not exist. Please provide a valid zone_id.";
            } else if (pgError.find("antenna_id") != std::string::npos) {
                details.userMessage = "The specified antenna does not exist. Please provide a valid antenna_id.";
            } else {
                details.userMessage = "Referenced entity does not exist. Please check your foreign key references.";
            }
            
            return details;
        }

        // Détection de violation d'unicité
        if (pgError.find("duplicate key") != std::string::npos ||
            pgError.find("unique constraint") != std::string::npos) {
            
            details.type = ErrorType::UNIQUE_VIOLATION;
            details.statusCode = k409Conflict;
            details.errorCode = "ERR_DB_UNIQUE_001";

            if (pgError.find("email") != std::string::npos) {
                details.userMessage = "This email address is already registered.";
            } else if (pgError.find("name") != std::string::npos) {
                details.userMessage = "An entity with this name already exists.";
            } else {
                details.userMessage = "This record already exists. Duplicate entries are not allowed.";
            }
            
            return details;
        }

        // Détection de not null violation
        if (pgError.find("null value") != std::string::npos ||
            pgError.find("violates not-null constraint") != std::string::npos) {
            
            details.type = ErrorType::VALIDATION_ERROR;
            details.statusCode = k400BadRequest;
            details.errorCode = "ERR_DB_NULL_001";
            details.userMessage = "Required field is missing. Please check your request data.";
            
            return details;
        }

        // Détection de connexion perdue
        if (pgError.find("connection") != std::string::npos ||
            pgError.find("could not connect") != std::string::npos) {
            
            details.type = ErrorType::DATABASE_ERROR;
            details.statusCode = k503ServiceUnavailable;
            details.errorCode = "ERR_DB_CONN_001";
            details.userMessage = "Database connection error. Please try again later.";
            
            return details;
        }

        // Erreur générique de base de données
        details.type = ErrorType::DATABASE_ERROR;
        details.statusCode = k500InternalServerError;
        details.errorCode = "ERR_DB_GENERIC_001";
        details.userMessage = "An internal database error occurred. Please contact support if the problem persists.";
        
        return details;
    }

    // Créer une réponse HTTP à partir des détails d'erreur
    static HttpResponsePtr createErrorResponse(const ErrorDetails& details) {
        Json::Value root;
        root["success"] = false;
        root["error"] = details.userMessage;
        root["errorCode"] = details.errorCode;
        
        // Ajouter timestamp
        auto now = trantor::Date::now();
        root["timestamp"] = now.toFormattedString(false);

        auto resp = HttpResponse::newHttpJsonResponse(root);
        resp->setStatusCode(details.statusCode);
        
        return resp;
    }

    // Logger l'erreur avec contexte
    static void logError(const std::string& context, const ErrorDetails& details) {
        LOG_ERROR << "[" << context << "] "
                  << "ErrorCode: " << details.errorCode << " | "
                  << "Type: " << errorTypeToString(details.type) << " | "
                  << "User Message: " << details.userMessage << " | "
                  << "Technical: " << details.technicalMessage;
    }

    // Gestion d'erreur 404 (ressource non trouvée)
    static HttpResponsePtr createNotFoundResponse(const std::string& resourceType, int id) {
        Json::Value root;
        root["success"] = false;
        root["error"] = resourceType + " with ID " + std::to_string(id) + " not found";
        root["errorCode"] = "ERR_NOT_FOUND_001";
        root["resourceType"] = resourceType;
        root["resourceId"] = id;
        
        auto now = trantor::Date::now();
        root["timestamp"] = now.toFormattedString(false);

        auto resp = HttpResponse::newHttpJsonResponse(root);
        resp->setStatusCode(k404NotFound);
        
        return resp;
    }

    // Gestion d'erreur générique
    static HttpResponsePtr createGenericErrorResponse(const std::string& message, 
                                                      HttpStatusCode code = k500InternalServerError) {
        Json::Value root;
        root["success"] = false;
        root["error"] = message;
        root["errorCode"] = "ERR_GENERIC_001";
        
        auto now = trantor::Date::now();
        root["timestamp"] = now.toFormattedString(false);

        auto resp = HttpResponse::newHttpJsonResponse(root);
        resp->setStatusCode(code);
        
        return resp;
    }

private:
    static std::string errorTypeToString(ErrorType type) {
        switch (type) {
            case ErrorType::DATABASE_ERROR: return "DATABASE_ERROR";
            case ErrorType::FOREIGN_KEY_VIOLATION: return "FOREIGN_KEY_VIOLATION";
            case ErrorType::UNIQUE_VIOLATION: return "UNIQUE_VIOLATION";
            case ErrorType::NOT_FOUND: return "NOT_FOUND";
            case ErrorType::VALIDATION_ERROR: return "VALIDATION_ERROR";
            case ErrorType::UNKNOWN_ERROR: return "UNKNOWN_ERROR";
            default: return "UNKNOWN";
        }
    }
};

#endif