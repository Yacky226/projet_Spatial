#pragma once
#include <drogon/HttpFilter.h>

using namespace drogon;

// Filtre HTTP pour gérer les headers CORS
class CorsFilter : public HttpFilter<CorsFilter> {
public:
    CorsFilter() {}
    
    // CORRECTION ICI : Utilisation de && au lieu de const &
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};