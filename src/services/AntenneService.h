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
    
    // ========== CLUSTERING BACKEND (Sprint 1 Optimization) ==========
    /**
     * Clustering backend optimisé utilisant ST_SnapToGrid de PostGIS
     * 
     * @param minLat, minLon, maxLat, maxLon - Bounding box de la vue
     * @param zoom - Niveau de zoom Leaflet (0-18) pour calculer la taille de grille
     * @param status - Filtre optionnel par statut (vide = tous)
     * @param technology - Filtre optionnel par technologie (vide = tous)
     * @param operator_id - Filtre optionnel par opérateur (0 = tous)
     * @param callback - Retourne GeoJSON avec clusters ou erreur
     */
    static void getClusteredAntennas(double minLat, double minLon, double maxLat, double maxLon, 
                                    int zoom, const std::string& status, 
                                    const std::string& technology, int operator_id,
                                    std::function<void(const Json::Value&, const std::string&)> callback);
    
    // ========== SIMPLIFIED COVERAGE (Sprint 4 Performance + Filtres) ==========
    /**
     * Calcul ultra-optimisé de la zone de couverture totale
     * Utilise ST_Union + ST_Simplify pour une navigation fluide
     * 
     * @param minLat, minLon, maxLat, maxLon - Bounding box de la vue
     * @param zoom - Niveau de zoom pour ajuster la simplification
     * @param operator_id - Filtre optionnel par opérateur (0 = tous)
     * @param technology - Filtre optionnel par technologie ("" = toutes)
     * @param callback - Retourne GeoJSON simplifié ou erreur
     */
    static void getSimplifiedCoverage(double minLat, double minLon, double maxLat, double maxLon, int zoom,
                                     int operator_id, const std::string& technology,
                                     std::function<void(const Json::Value&, const std::string&)> callback);
};