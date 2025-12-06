#include "CorsFilter.h"

using namespace drogon;

// Filtrage des requêtes pour gérer CORS
void CorsFilter::doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) {
    // Gérer la requête "Pre-flight" (OPTIONS)
    if (req->method() == Options) {
        LOG_INFO << "🔄 Handling CORS preflight request for: " << req->path();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
        resp->addHeader("Access-Control-Max-Age", "86400");

        LOG_INFO << "✅ CORS preflight response sent";
        fcb(resp);
        return;
    }

    // Pour les autres requêtes, continuer la chaîne
    // Les headers CORS seront ajoutés par le PostHandlingAdvice
    fccb();
}