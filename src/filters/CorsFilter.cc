#include "CorsFilter.h"

using namespace drogon;

void CorsFilter::doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) {
    // 1. Gérer la requête "Pre-flight" (OPTIONS)
    if (req->method() == Options) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
        resp->addHeader("Access-Control-Max-Age", "86400");
        
        fcb(resp); // On renvoie la réponse immédiatement
        return;
    }

    // 2. Pour les autres requêtes (GET, POST...), on laisse passer.
    // Les headers CORS seront ajoutés par le PostHandlingAdvice dans main.cpp
    fccb();
}