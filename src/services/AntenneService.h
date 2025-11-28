#pragma once
#include "../models/Antenne.h"
#include "../utils/PaginationMeta.h"  // Utilisez le fichier commun
#include <drogon/drogon.h>
#include <vector>
#include <string>
#include <functional>

class AntenneService {
public:
    // ========== CRUD CLASSIQUE ==========
    static void create(const Antenna &antenne, std::function<void(const std::string&)> callback);
    
    // Version sans pagination (compatibilité rétroactive)
    static void getAll(std::function<void(const std::vector<Antenna>&, const std::string&)> callback);
    
    // Version avec pagination
    static void getAllPaginated(int page, int pageSize,
                               std::function<void(const std::vector<Antenna>&, 
                                                const PaginationMeta&,
                                                const std::string&)> callback);
    
    static void getById(int id, std::function<void(const Antenna&, const std::string&)> callback);
    static void update(const Antenna &antenne, std::function<void(const std::string&)> callback);
    static void remove(int id, std::function<void(const std::string&)> callback);

    // ========== RECHERCHE GÉOGRAPHIQUE ==========
    static void getInRadius(double lat, double lon, double radiusMeters, 
                           std::function<void(const std::vector<Antenna>&, const std::string&)> callback);
    
    // Recherche avec pagination
    static void getInRadiusPaginated(double lat, double lon, double radiusMeters,
                                    int page, int pageSize,
                                    std::function<void(const std::vector<Antenna>&,
                                                     const PaginationMeta&,
                                                     const std::string&)> callback);

    // ========== GEOJSON POUR LEAFLET ==========
    
    // GeoJSON complet (tous les points)
    static void getAllGeoJSON(std::function<void(const Json::Value&, const std::string&)> callback);
    
    // GeoJSON paginé (optimisé pour grandes quantités)
    static void getAllGeoJSONPaginated(int page, int pageSize,
                                      std::function<void(const Json::Value&,
                                                       const PaginationMeta&,
                                                       const std::string&)> callback);
    
    // GeoJSON dans un rayon (pour carte dynamique)
    static void getGeoJSONInRadius(double lat, double lon, double radiusMeters,
                                  std::function<void(const Json::Value&, const std::string&)> callback);
    
    // GeoJSON dans un rayon avec pagination
    static void getGeoJSONInRadiusPaginated(double lat, double lon, double radiusMeters,
                                           int page, int pageSize,
                                           std::function<void(const Json::Value&,
                                                            const PaginationMeta&,
                                                            const std::string&)> callback);
    
    // GeoJSON dans une bbox (bounding box pour Leaflet)
    static void getGeoJSONInBBox(double minLat, double minLon, double maxLat, double maxLon,
                                std::function<void(const Json::Value&, const std::string&)> callback);
    
    static void getCoverageByOperator(int operator_id, double minLat, double minLon, double maxLat, double maxLon,
                                     std::function<void(const Json::Value&, const std::string&)> callback);
                                    
    static void getVoronoiDiagram(
        std::function<void(const Json::Value&, const std::string&)> callback);
};