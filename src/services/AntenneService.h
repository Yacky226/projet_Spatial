#pragma once
#include "../models/Antenne.h"
#include <drogon/drogon.h>
#include <vector>
#include <string>
#include <functional>

class AntenneService {
public:
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