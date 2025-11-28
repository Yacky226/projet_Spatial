# API Antennes 5G - Documentation Complète

## 📋 Table des matières

1. [Vue d'ensemble](#-vue-densemble)
2. [Architecture](#-architecture)
3. [Modèles de données](#-modèles-de-données)
4. [Endpoints API](#-endpoints-api)
   - [Antennes](#-antennes)
   - [Obstacles](#-obstacles)
   - [Zones](#-zones)
   - [Opérateurs](#-opérateurs)
   - [Relations](#-relations)
   - [📊 Couverture (NOUVEAU)](#-couverture)
   - [📡 Simulation Radio (NOUVEAU)](#-simulation-radio)
   - [🎯 Optimisation (NOUVEAU)](#-optimisation)
5. [Pagination](#-pagination)
6. [Validation](#-validation)
7. [Gestion des erreurs](#-gestion-des-erreurs)
8. [Installation](#-installation)
9. [Exemples d'utilisation](#-exemples-dutilisation)

---

## 🎯 Vue d'ensemble

API REST pour la gestion et la visualisation d'antennes 5G avec support PostGIS pour les fonctionnalités géospatiales avancées.

### Fonctionnalités principales

- ✅ CRUD complet pour antennes, obstacles, zones et opérateurs
- ✅ Recherche géographique (rayon, bounding box)
- ✅ Export GeoJSON pour intégration Leaflet/Mapbox
- ✅ **🆕 Analyse de couverture réseau (population, zones blanches)**
- ✅ **🆕 Simulation de signal radio (FSPL, atténuation)**
- ✅ **🆕 Optimisation de placement d'antennes (Greedy, K-means)**
- ✅ **🆕 Diagrammes de Voronoi pour zones de service**
- ✅ Pagination optimisée
- ✅ Validation complète des données
- ✅ Gestion avancée des erreurs PostgreSQL
- ✅ Support PostGIS pour géométries complexes

---

## 🏗️ Architecture

```
src/
├── controllers/        # Gestion des requêtes HTTP
│   ├── AntenneController.cc
│   ├── ObstacleController.cc
│   ├── ZoneController.cc
│   ├── OperatorController.cc
│   ├── AntennaZoneController.cc
│   ├── ZoneObstacleController.cc
│   ├── CoverageController.cc      # 🆕 Analyse de couverture
│   ├── SimulationController.cc    # 🆕 Simulation radio
│   └── OptimizationController.cc  # 🆕 Placement optimal
│
├── services/          # Logique métier et accès DB
│   ├── AntenneService.cc
│   ├── ObstacleService.cc
│   ├── ZoneService.cc
│   ├── OperatorService.cc
│   ├── AntennaZoneService.cc
│   ├── ZoneObstacleService.cc
│   ├── CoverageService.cc         # 🆕 Calculs de couverture
│   ├── SimulationService.cc       # 🆕 Modèle de propagation
│   └── OptimizationService.cc     # 🆕 Algorithmes d'optimisation
│
├── models/           # Structures de données
│   ├── Antenne.h
│   ├── Obstacle.h
│   ├── Zone.h
│   ├── Operator.h
│   ├── CoverageResult.h           # 🆕
│   ├── SignalSimulation.h         # 🆕
│   └── OptimizationRequest.h      # 🆕
│
└── utils/            # Utilitaires
    ├── Validator.h
    ├── ErrorHandler.h
    ├── PaginationHelper.h
    └── PaginationMeta.h
```

---

## 📊 Modèles de données

### Antenne (Antenna)

```cpp
struct Antenna {
    int id;
    double latitude;           // -90 à +90
    double longitude;          // -180 à +180
    double coverage_radius;    // En mètres (max 50000m)
    std::string status;        // active, inactive, maintenance, planned
    std::string technology;    // 4G, 5G, 5G-SA, 5G-NSA
    std::string installation_date;  // Format ISO 8601
    int operator_id;
};
```

### Obstacle

```cpp
struct ObstacleModel {
    int id;
    std::string type;          // batiment, vegetation, relief
    std::string geom_type;     // POINT, POLYGON, LINESTRING
    std::string wkt_geometry;  // Format WKT (Well-Known Text)
};
```

### Zone

```cpp
struct Zone {
    int id;
    std::string name;
    std::string wkt_geometry;  // POLYGON en WKT
    double area;               // Calculé automatiquement (m²)
    double population;         // 🆕 Population totale
    double density;            // 🆕 Densité (hab/km²)
};
```

### Operator

```cpp
struct Operator {
    int id;
    std::string name;
    std::string contact_email;
    std::string contact_phone;
};
```

### 🆕 CoverageResult

```cpp
struct CoverageResult {
    int antenna_id;
    double covered_population;
    double coverage_percent;
    std::string geojson;       // Zone de couverture
};
```

### 🆕 SignalSimulation

```cpp
struct SignalSimulation {
    double latitude;
    double longitude;
    double signal_strength_dbm;
    std::string quality;       // excellent, good, fair, poor
    int nearest_antenna_id;
    double distance_meters;
};
```

### 🆕 OptimizationRequest

```cpp
struct OptimizationRequest {
    int zone_id;
    int antennas_count;
    double radius;
    std::string technology;
    std::string algorithm;     // greedy, kmeans
};
```

---

## 🔌 Endpoints API

### 📡 Antennes

#### **POST** `/api/antennes`

Créer une nouvelle antenne.

**Body:**

```json
{
  "latitude": 48.8566,
  "longitude": 2.3522,
  "coverage_radius": 5000,
  "status": "active",
  "technology": "5G",
  "installation_date": "2024-01-15",
  "operator_id": 1
}
```

**Validations:**

- Latitude: -90 à +90
- Longitude: -180 à +180
- Coverage radius: > 0 et ≤ 50000m
- Status: `active`, `inactive`, `maintenance`, `planned`
- Technology: `4G`, `5G`, `5G-SA`, `5G-NSA`

**Response (201):**

```json
{
  "success": true,
  "message": "Antenna created successfully",
  "timestamp": "2024-11-23T10:30:00Z"
}
```

---

#### **GET** `/api/antennes`

Lister toutes les antennes (avec pagination optionnelle).

**Query params:**

- `page` (optionnel): Numéro de page (défaut: 1)
- `pageSize` (optionnel): Éléments par page (défaut: 20, max: 100)

**Response sans pagination:**

```json
{
  "success": true,
  "data": [
    {
      "id": 1,
      "latitude": 48.8566,
      "longitude": 2.3522,
      "coverage_radius": 5000,
      "status": "active",
      "technology": "5G",
      "installation_date": "2024-01-15",
      "operator_id": 1
    }
  ],
  "count": 1,
  "timestamp": "2024-11-23T10:30:00Z"
}
```

**Response avec pagination:**

```json
{
  "success": true,
  "data": [...],
  "pagination": {
    "currentPage": 1,
    "pageSize": 20,
    "totalItems": 150,
    "totalPages": 8,
    "hasNext": true,
    "hasPrev": false,
    "links": {
      "self": "/api/antennes?pageSize=20&page=1",
      "first": "/api/antennes?pageSize=20&page=1",
      "last": "/api/antennes?pageSize=20&page=8",
      "next": "/api/antennes?pageSize=20&page=2"
    }
  },
  "timestamp": "2024-11-23T10:30:00Z"
}
```

---

#### **GET** `/api/antennes/{id}`

Obtenir une antenne par ID.

**Response (200):**

```json
{
  "success": true,
  "data": {
    "id": 1,
    "latitude": 48.8566,
    "longitude": 2.3522,
    "coverage_radius": 5000,
    "status": "active",
    "technology": "5G",
    "installation_date": "2024-01-15",
    "operator_id": 1
  },
  "timestamp": "2024-11-23T10:30:00Z"
}
```

---

#### **PUT** `/api/antennes/{id}`

Mettre à jour une antenne.

**Body (champs optionnels):**

```json
{
  "status": "maintenance",
  "coverage_radius": 6000
}
```

**Response (200):**

```json
{
  "success": true,
  "message": "Antenna updated successfully",
  "antennaId": 1,
  "timestamp": "2024-11-23T10:30:00Z"
}
```

---

#### **DELETE** `/api/antennes/{id}`

Supprimer une antenne.

**Response (204 No Content)**

---

#### **GET** `/api/antennes/search/radius`

Rechercher des antennes dans un rayon.

**Query params:**

- `lat`: Latitude du centre
- `lon`: Longitude du centre
- `radius`: Rayon en mètres
- `page` (optionnel): Pagination
- `pageSize` (optionnel): Pagination

**Exemple:**

```
GET /api/antennes/search/radius?lat=48.8566&lon=2.3522&radius=10000
```

**Response:**

```json
{
  "success": true,
  "data": [...],
  "count": 5,
  "timestamp": "2024-11-23T10:30:00Z"
}
```

---

#### **GET** `/api/antennes/geojson`

Export GeoJSON de toutes les antennes.

**Query params:**

- `page` (optionnel)
- `pageSize` (optionnel)

**Response:**

```json
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Point",
        "coordinates": [2.3522, 48.8566]
      },
      "properties": {
        "id": 1,
        "status": "active",
        "technology": "5G",
        "coverage_radius": 5000,
        "operator_id": 1
      }
    }
  ]
}
```

---

#### **GET** `/api/antennes/geojson/radius`

GeoJSON des antennes dans un rayon.

**Query params:**

- `lat`, `lon`, `radius`
- `page`, `pageSize` (optionnels)

---

#### **GET** `/api/antennes/geojson/bbox`

GeoJSON des antennes dans une bounding box.

**Query params:**

- `minLat`: Latitude minimale
- `minLon`: Longitude minimale
- `maxLat`: Latitude maximale
- `maxLon`: Longitude maximale

**Exemple:**

```
GET /api/antennes/geojson/bbox?minLat=48.8&minLon=2.3&maxLat=48.9&maxLon=2.4
```

---

#### 🆕 **GET** `/api/antennes/voronoi`

Obtenir le diagramme de Voronoi des antennes actives.

**Description:** Calcule les zones de service optimales pour chaque antenne (polygones de Voronoi).

**Response (200):**

```json
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Polygon",
        "coordinates": [[[2.33, 48.86], [2.35, 48.87], ...]]
      },
      "properties": {
        "antenna_id": 1,
        "status": "active",
        "technology": "5G"
      }
    }
  ]
}
```

**Utilisation Leaflet:**

```javascript
fetch("/api/antennes/voronoi")
  .then((res) => res.json())
  .then((data) => {
    L.geoJSON(data, {
      style: (feature) => ({
        fillColor:
          feature.properties.technology === "5G" ? "#00ff00" : "#ff0000",
        fillOpacity: 0.3,
        color: "#333",
        weight: 2,
      }),
    }).addTo(map);
  });
```

---

### 🏢 Obstacles

#### **POST** `/api/obstacles`

Créer un obstacle.

**Body:**

```json
{
  "type": "batiment",
  "geom_type": "POLYGON",
  "wkt": "POLYGON((2.35 48.85, 2.36 48.85, 2.36 48.86, 2.35 48.86, 2.35 48.85))"
}
```

**Validations:**

- Type: `batiment`, `vegetation`, `relief`
- Geom type: `POINT`, `POLYGON`, `LINESTRING`
- WKT: Format valide

---

#### **GET** `/api/obstacles`

Lister tous les obstacles (pagination supportée).

---

#### **GET** `/api/obstacles/{id}`

Obtenir un obstacle par ID.

---

#### **PUT** `/api/obstacles/{id}`

Mettre à jour un obstacle.

---

#### **DELETE** `/api/obstacles/{id}`

Supprimer un obstacle.

---

#### **GET** `/api/obstacles/geojson`

Export GeoJSON des obstacles (pagination supportée).

**Response:**

```json
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Polygon",
        "coordinates": [[[2.35, 48.85], [2.36, 48.85], ...]]
      },
      "properties": {
        "id": 1,
        "type": "batiment",
        "geom_type": "POLYGON"
      }
    }
  ]
}
```

---

### 🗺️ Zones

#### **POST** `/api/zones`

Créer une zone géographique.

**Body:**

```json
{
  "name": "Zone Centre Paris",
  "wkt": "POLYGON((2.33 48.86, 2.34 48.86, 2.34 48.87, 2.33 48.87, 2.33 48.86))",
  "population": 50000,
  "density": 10000
}
```

---

#### **GET** `/api/zones`

Lister toutes les zones (pagination supportée).

---

#### **GET** `/api/zones/{id}`

Obtenir une zone par ID.

---

#### **PUT** `/api/zones/{id}`

Mettre à jour une zone.

---

#### **DELETE** `/api/zones/{id}`

Supprimer une zone.

---

#### **GET** `/api/zones/geojson`

Export GeoJSON des zones (pagination supportée).

---

### 📞 Opérateurs

#### **POST** `/api/operators`

Créer un opérateur.

**Body:**

```json
{
  "name": "Orange",
  "contact_email": "contact@orange.fr",
  "contact_phone": "+33123456789"
}
```

---

#### **GET** `/api/operators`

Lister tous les opérateurs (pagination supportée).

---

#### **GET** `/api/operators/{id}`

Obtenir un opérateur par ID.

---

#### **PUT** `/api/operators/{id}`

Mettre à jour un opérateur.

---

#### **DELETE** `/api/operators/{id}`

Supprimer un opérateur.

---

### 🔗 Relations

#### **POST** `/api/antenna-zones`

Associer une antenne à une zone.

**Body:**

```json
{
  "antenna_id": 1,
  "zone_id": 2
}
```

---

#### **GET** `/api/antenna-zones`

Lister toutes les associations antenne-zone.

---

#### **DELETE** `/api/antenna-zones/{antenna_id}/{zone_id}`

Supprimer une association.

---

#### **POST** `/api/zone-obstacles`

Associer un obstacle à une zone.

---

#### **GET** `/api/zone-obstacles`

Lister toutes les associations zone-obstacle.

---

#### **DELETE** `/api/zone-obstacles/{zone_id}/{obstacle_id}`

Supprimer une association.

---

## 🆕 📊 Couverture

### **GET** `/api/coverage/antenna/{id}`

Calculer la couverture d'une antenne spécifique.

**Description:** Retourne la population couverte, le taux de couverture et la zone géographique couverte.

**Response (200):**

```json
{
  "success": true,
  "data": {
    "antenna_id": 1,
    "covered_population": 15000,
    "coverage_percent": 75.5,
    "coverage_area_km2": 78.5,
    "geojson": {
      "type": "Polygon",
      "coordinates": [...]
    }
  },
  "timestamp": "2024-11-29T12:00:00Z"
}
```

**Exemple cURL:**

```bash
curl http://localhost:8080/api/coverage/antenna/1
```

---

### **GET** `/api/coverage/white-zones/{zone_id}`

Obtenir les zones blanches (non couvertes) en GeoJSON.

**Description:** Retourne les polygones des zones sans couverture réseau.

**Response (200):**

```json
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Polygon",
        "coordinates": [[[2.33, 48.85], ...]]
      },
      "properties": {
        "zone_id": 1,
        "population": 5000,
        "area_km2": 12.3
      }
    }
  ]
}
```

**Utilisation Leaflet:**

```javascript
fetch("/api/coverage/white-zones/1")
  .then((res) => res.json())
  .then((data) => {
    L.geoJSON(data, {
      style: {
        fillColor: "#ff0000",
        fillOpacity: 0.4,
        color: "#ff0000",
        weight: 2,
      },
    })
      .addTo(map)
      .bindPopup("Zone blanche - Pas de couverture");
  });
```

---

### **GET** `/api/coverage/stats`

Statistiques globales de couverture.

**Description:** Vue d'ensemble : nombre d'antennes, taux de couverture, population couverte.

**Response (200):**

```json
{
  "success": true,
  "data": {
    "antennas": {
      "total": 150,
      "active": 142,
      "by_technology": {
        "5G": 95,
        "4G": 55
      }
    },
    "coverage": {
      "total_population": 500000,
      "covered_population": 425000,
      "coverage_percent": 85.0,
      "total_area_km2": 1200.5
    }
  },
  "timestamp": "2024-11-29T12:00:00Z"
}
```

**Exemple cURL:**

```bash
curl http://localhost:8080/api/coverage/stats
```

---

## 🆕 📡 Simulation Radio

### **GET** `/api/simulation/check`

Simuler la qualité du signal à un point donné.

**Description:** Calcule le signal reçu en dBm en tenant compte de la distance (FSPL) et des obstacles.

**Query params:**

- `lat`: Latitude du point
- `lon`: Longitude du point

**Exemple:**

```
GET /api/simulation/check?lat=48.8566&lon=2.3522
```

**Response (200):**

```json
{
  "success": true,
  "data": {
    "latitude": 48.8566,
    "longitude": 2.3522,
    "signal_strength_dbm": -75.3,
    "quality": "good",
    "nearest_antenna": {
      "id": 5,
      "distance_meters": 1250,
      "technology": "5G",
      "status": "active"
    },
    "attenuation": {
      "free_space_loss_db": 95.2,
      "obstacle_loss_db": 12.5,
      "total_loss_db": 107.7
    }
  },
  "timestamp": "2024-11-29T12:00:00Z"
}
```

**Qualité du signal:**

| Signal (dBm) | Qualité     | Description                 |
| ------------ | ----------- | --------------------------- |
| > -60        | `excellent` | Signal très fort            |
| -60 à -75    | `good`      | Bon signal                  |
| -75 à -90    | `fair`      | Signal acceptable           |
| < -90        | `poor`      | Signal faible ou inexistant |

**Exemple JavaScript (carte interactive):**

```javascript
map.on("click", function (e) {
  fetch(`/api/simulation/check?lat=${e.latlng.lat}&lon=${e.latlng.lng}`)
    .then((res) => res.json())
    .then((data) => {
      const quality = data.data.quality;
      const color = {
        excellent: "#00ff00",
        good: "#90ee90",
        fair: "#ffff00",
        poor: "#ff0000",
      }[quality];

      L.circleMarker(e.latlng, {
        color: color,
        radius: 8,
      })
        .addTo(map)
        .bindPopup(
          `Signal: ${data.data.signal_strength_dbm} dBm<br>Qualité: ${quality}`
        );
    });
});
```

---

## 🆕 🎯 Optimisation

### **POST** `/api/optimization/optimize`

Optimiser le placement d'antennes dans une zone.

**Description:** Trouve les meilleurs emplacements pour maximiser la couverture de population.

**Body:**

```json
{
  "zone_id": 1,
  "antennas_count": 5,
  "radius": 2000,
  "technology": "5G",
  "algorithm": "kmeans"
}
```

**Paramètres:**

- `zone_id`: ID de la zone à couvrir
- `antennas_count`: Nombre d'antennes à placer (1-100)
- `radius`: Rayon de couverture en mètres
- `technology`: `4G`, `5G`, `5G-SA`, `5G-NSA`
- `algorithm`: **`greedy`** (Monte Carlo) ou **`kmeans`** (clustering)

**Response (200):**

```json
{
  "success": true,
  "algorithm": "kmeans",
  "count": 5,
  "candidates": [
    {
      "latitude": 48.8575,
      "longitude": 2.3498,
      "estimated_population": 12500,
      "score": 12500
    },
    {
      "latitude": 48.859,
      "longitude": 2.351,
      "estimated_population": 11800,
      "score": 11800
    }
  ],
  "timestamp": "2024-11-29T12:00:00Z"
}
```

**Algorithmes disponibles:**

#### 1. **Greedy (Monte Carlo)**

- Génère N candidats aléatoires
- Sélectionne les meilleurs en fonction de la population couverte
- Rapide, mais peut manquer l'optimum global

```json
{
  "algorithm": "greedy"
}
```

#### 2. **K-means Clustering** (Recommandé)

- Analyse la distribution de population
- Place les antennes aux centres de gravité des clusters
- Plus intelligent, tient compte de la densité

```json
{
  "algorithm": "kmeans"
}
```

**Exemple cURL:**

```bash
curl -X POST http://localhost:8080/api/optimization/optimize \
  -H "Content-Type: application/json" \
  -d '{
    "zone_id": 1,
    "antennas_count": 5,
    "radius": 2000,
    "technology": "5G",
    "algorithm": "kmeans"
  }'
```

**Visualisation Leaflet:**

```javascript
fetch("/api/optimization/optimize", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({
    zone_id: 1,
    antennas_count: 5,
    radius: 2000,
    technology: "5G",
    algorithm: "kmeans",
  }),
})
  .then((res) => res.json())
  .then((data) => {
    data.candidates.forEach((candidate) => {
      L.circle([candidate.latitude, candidate.longitude], {
        radius: 2000,
        color: "#0000ff",
        fillColor: "#0066ff",
        fillOpacity: 0.3,
      })
        .addTo(map)
        .bindPopup(`Population: ${candidate.estimated_population}`);
    });
  });
```

---

## 📄 Pagination

La pagination est supportée sur tous les endpoints de liste.

### Format de réponse paginée

```json
{
  "success": true,
  "data": [...],
  "pagination": {
    "currentPage": 2,
    "pageSize": 20,
    "totalItems": 150,
    "totalPages": 8,
    "hasNext": true,
    "hasPrev": true,
    "links": {
      "self": "/api/antennes?pageSize=20&page=2",
      "first": "/api/antennes?pageSize=20&page=1",
      "last": "/api/antennes?pageSize=20&page=8",
      "next": "/api/antennes?pageSize=20&page=3",
      "prev": "/api/antennes?pageSize=20&page=1"
    }
  }
}
```

### Limites

- `pageSize` max: 100
- `page` min: 1

---

## ✅ Validation

### Coordonnées GPS

- **Latitude**: `-90.0` à `+90.0`
- **Longitude**: `-180.0` à `+180.0`

### Antennes

- **Coverage radius**: `> 0` et `≤ 50000` mètres
- **Status**: `active`, `inactive`, `maintenance`, `planned`
- **Technology**: `4G`, `5G`, `5G-SA`, `5G-NSA`

### Obstacles

- **Type**: `batiment`, `vegetation`, `relief`
- **Geom type**: `POINT`, `POLYGON`, `LINESTRING`

### Opérateurs

- **Email**: Format RFC 5322
- **Téléphone**: Format international `+[country][number]`

### Zones

- **Name**: Non vide, max 255 caractères
- **WKT**: Format PostGIS valide
- **Population**: ≥ 0
- **Density**: ≥ 0 (hab/km²)

### 🆕 Optimisation

- **antennas_count**: 1 à 100
- **radius**: > 0 et ≤ 50000 mètres
- **algorithm**: `greedy` ou `kmeans`

---

## ❌ Gestion des erreurs

### Format de réponse d'erreur

```json
{
  "success": false,
  "error": {
    "code": "23505",
    "type": "unique_violation",
    "message": "Duplicate key value violates unique constraint",
    "detail": "Key (email)=(test@example.com) already exists.",
    "hint": "Please use a different email address",
    "severity": "ERROR"
  },
  "timestamp": "2024-11-23T10:30:00Z"
}
```

### Codes HTTP

| Code | Signification                   |
| ---- | ------------------------------- |
| 200  | Succès                          |
| 201  | Créé                            |
| 204  | Supprimé (pas de contenu)       |
| 400  | Erreur de validation            |
| 404  | Ressource non trouvée           |
| 409  | Conflit (duplicate, contrainte) |
| 500  | Erreur serveur                  |

### Erreurs de validation

```json
{
  "success": false,
  "errors": [
    {
      "field": "latitude",
      "message": "Latitude must be between -90 and +90 degrees"
    },
    {
      "field": "technology",
      "message": "Technology must be one of: 4G, 5G, 5G-SA, 5G-NSA"
    }
  ]
}
```

---

## 🚀 Installation

### Prérequis

- Docker & Docker Compose
- PostgreSQL 14+ avec PostGIS 3.3+
- C++17
- Drogon Framework 1.8+
- jsoncpp

### Configuration Docker

```yaml
version: "3.8"
services:
  postgres:
    image: postgis/postgis:14-3.3
    environment:
      POSTGRES_DB: antennes_5g
      POSTGRES_USER: admin
      POSTGRES_PASSWORD: password
    ports:
      - "5432:5432"
    volumes:
      - postgres_data:/var/lib/postgresql/data
      - ./init.sql:/docker-entrypoint-initdb.d/init.sql

  api:
    build: .
    ports:
      - "8080:8080"
    depends_on:
      - postgres
    environment:
      DB_HOST: postgres
      DB_PORT: 5432
      DB_NAME: antennes_5g
      DB_USER: admin
      DB_PASSWORD: password

volumes:
  postgres_data:
```

### Build

```bash
# Cloner le projet
git clone <repo-url>
cd antennes-5g

# Build et lancer avec Docker Compose
docker-compose up --build

# Ou build manuel
docker build -t antennes-5g-api .
docker run -p 8080:8080 antennes-5g-api
```

### Configuration

Fichier `config.json`:

```json
{
  "app": {
    "threads_num": 4,
    "port": 8080,
    "server_header_field": "Antennes-5G-API"
  },
  "db": {
    "host": "postgres",
    "port": 5432,
    "dbname": "antennes_5g",
    "user": "admin",
    "password": "password",
    "client_encoding": "UTF8",
    "connection_number": 10
  }
}
```

### Base de données

**Initialisation PostGIS** (`init.sql`):

```sql
CREATE EXTENSION IF NOT EXISTS postgis;

CREATE TABLE zone (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    geom GEOMETRY(Polygon, 4326) NOT NULL,
    population INTEGER DEFAULT 0,
    density FLOAT DEFAULT 0
);

CREATE INDEX idx_zone_geom ON zone USING GIST(geom);
```

---

## 💡 Exemples d'utilisation

### 1. Créer une antenne avec cURL

```bash
curl -X POST http://localhost:8080/api/antennes \
  -H "Content-Type: application/json" \
  -d '{
    "latitude": 48.8566,
    "longitude": 2.3522,
    "coverage_radius": 5000,
    "status": "active",
    "technology": "5G",
    "installation_date": "2024-01-15",
    "operator_id": 1
  }'
```

---

### 2. Recherche dans un rayon

```bash
curl "http://localhost:8080/api/antennes/search/radius?lat=48.8566&lon=2.3522&radius=10000"
```

---

### 3. Export GeoJSON pour Leaflet

```javascript
// Afficher toutes les antennes
fetch("http://localhost:8080/api/antennes/geojson")
  .then((res) => res.json())
  .then((data) => {
    L.geoJSON(data, {
      pointToLayer: (feature, latlng) => {
        return L.circle(latlng, {
          radius: feature.properties.coverage_radius,
          color: feature.properties.status === "active" ? "green" : "red",
          fillOpacity: 0.3,
        });
      },
      onEachFeature: (feature, layer) => {
        layer.bindPopup(`
          <b>Antenne ${feature.properties.id}</b><br>
          Technologie: ${feature.properties.technology}<br>
          Status: ${feature.properties.status}<br>
          Rayon: ${feature.properties.coverage_radius}m
        `);
      },
    }).addTo(map);
  });
```

---

### 4. Afficher les zones blanches

```javascript
fetch("/api/coverage/white-zones/1")
  .then((res) => res.json())
  .then((data) => {
    L.geoJSON(data, {
      style: {
        fillColor: "#ff0000",
        fillOpacity: 0.5,
        color: "#cc0000",
        weight: 2,
      },
    })
      .addTo(map)
      .bindPopup("Zone blanche - Pas de couverture");
  });
```

---

### 5. Simulation de signal interactive

```javascript
let simulationLayer = L.layerGroup().addTo(map);

map.on("click", function (e) {
  fetch(`/api/simulation/check?lat=${e.latlng.lat}&lon=${e.latlng.lng}`)
    .then((res) => res.json())
    .then((data) => {
      const signal = data.data;
      const colors = {
        excellent: "#00ff00",
        good: "#90ee90",
        fair: "#ffff00",
        poor: "#ff0000",
      }[signal.quality];

      L.circleMarker(e.latlng, {
        radius: 10,
        fillColor: colors[signal.quality],
        fillOpacity: 0.8,
        color: "#000",
        weight: 1,
      })
        .addTo(simulationLayer)
        .bindPopup(
          `
          <b>Simulation de signal</b><br>
          Signal: ${signal.signal_strength_dbm.toFixed(1)} dBm<br>
          Qualité: <span style="color:${colors[signal.quality]}">${
            signal.quality
          }</span><br>
          Distance antenne: ${signal.nearest_antenna.distance_meters.toFixed(
            0
          )}m<br>
          Technologie: ${signal.nearest_antenna.technology}
        `
        )
        .openPopup();
    });
});

// Bouton pour effacer les simulations
L.easyButton("🗑️", function () {
  simulationLayer.clearLayers();
}).addTo(map);
```

---

### 6. Optimisation de placement

```javascript
document.getElementById("optimizeBtn").addEventListener("click", function () {
  const zoneId = document.getElementById("zoneSelect").value;
  const count = document.getElementById("antennaCount").value;
  const algorithm = document.getElementById("algorithmSelect").value;

  fetch("/api/optimization/optimize", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      zone_id: parseInt(zoneId),
      antennas_count: parseInt(count),
      radius: 2000,
      technology: "5G",
      algorithm: algorithm,
    }),
  })
    .then((res) => res.json())
    .then((data) => {
      // Afficher les candidats
      let candidatesLayer = L.layerGroup().addTo(map);

      data.candidates.forEach((candidate, index) => {
        L.marker([candidate.latitude, candidate.longitude], {
          icon: L.divIcon({
            className: "antenna-candidate",
            html: `<div style="background:blue;color:white;padding:5px;border-radius:50%;">${
              index + 1
            }</div>`,
          }),
        }).addTo(candidatesLayer).bindPopup(`
          <b>Candidat #${index + 1}</b><br>
          Population: ${candidate.estimated_population.toFixed(0)}<br>
          Score: ${candidate.score}
        `);

        // Rayon de couverture
        L.circle([candidate.latitude, candidate.longitude], {
          radius: 2000,
          color: "#0066ff",
          fillOpacity: 0.1,
        }).addTo(candidatesLayer);
      });

      alert(
        `${data.candidates.length} emplacements optimaux trouvés avec l'algorithme ${algorithm}`
      );
    });
});
```

---

### 7. Dashboard de statistiques

```javascript
fetch("/api/coverage/stats")
  .then((res) => res.json())
  .then((data) => {
    const stats = data.data;

    document.getElementById("totalAntennas").textContent = stats.antennas.total;
    document.getElementById("activeAntennas").textContent =
      stats.antennas.active;
    document.getElementById("coverage5G").textContent =
      stats.antennas.by_technology["5G"];
    document.getElementById("coveragePercent").textContent =
      stats.coverage.coverage_percent.toFixed(1) + "%";
    document.getElementById("coveredPopulation").textContent =
      (stats.coverage.covered_population / 1000).toFixed(0) + "K";

    // Diagramme circulaire
    const ctx = document.getElementById("coverageChart").getContext("2d");
    new Chart(ctx, {
      type: "pie",
      data: {
        labels: ["Couvert", "Non couvert"],
        datasets: [
          {
            data: [
              stats.coverage.covered_population,
              stats.coverage.total_population -
                stats.coverage.covered_population,
            ],
            backgroundColor: ["#00ff00", "#ff0000"],
          },
        ],
      },
    });
  });
```

---

### 8. Pagination

```bash
# Page 2, 50 éléments par page
curl "http://localhost:8080/api/antennes?page=2&pageSize=50"
```

---

## 📝 Notes techniques

### PostGIS

L'API utilise PostGIS pour:

- ✅ Calcul de distances géographiques (`ST_Distance`)
- ✅ Recherche dans un rayon (`ST_DWithin`)
- ✅ Recherche dans une bbox (`ST_MakeEnvelope`)
- ✅ Calcul d'aires (`ST_Area`)
- ✅ Export GeoJSON (`ST_AsGeoJSON`)
- ✅ **Diagrammes de Voronoi** (`ST_VoronoiPolygons`)
- ✅ **Intersection de zones** (`ST_Intersection`)
- ✅ **Buffer/Rayon de couverture** (`ST_Buffer`)
- ✅ **Génération de points aléatoires** (`ST_GeneratePoints`)

### Modèles de propagation

#### Free Space Path Loss (FSPL)

```
FSPL (dB) = 20·log₁₀(d) + 20·log₁₀(f) + 32.45
```

- `d`: distance en km
- `f`: fréquence en MHz (3500 MHz pour 5G)

#### Atténuation par obstacles

| Type       | Perte (dB) |
| ---------- | ---------- |
| Bâtiment   | 15-30      |
| Végétation | 5-10       |
| Relief     | 10-20      |

### Algorithmes d'optimisation

#### K-means Clustering

1. Génère des points pondérés par la densité de population
2. Initialisation K-means++ (évite les clusters vides)
3. Itère jusqu'à convergence (max 50 itérations)
4. Calcule la population couverte pour chaque centroïde

**Complexité:** O(n·k·i) où n = points, k = clusters, i = itérations

#### Greedy Monte Carlo

1. Génère 1000 candidats aléatoires
2. Calcule la population pour chaque candidat
3. Retourne les K meilleurs

**Complexité:** O(n·k) où n = candidats, k = antennes

### Performance

- ✅ Index spatial sur colonnes géométriques (`GIST`)
- ✅ Pagination obligatoire au-delà de 100 éléments
- ✅ Pool de connexions PostgreSQL (10 connexions)
- ✅ Requêtes asynchrones (Drogon ORM)
- ⚠️ Cache recommandé pour `/coverage/stats` (calculs lourds)
- ⚠️ Optimisation K-means peut prendre 5-10s pour grandes zones

### Limites connues

- Optimisation K-means : max 100 antennes
- Simulation : calcule uniquement l'antenne la plus proche
- Zones blanches : nécessite des données de population fiables
- Diagramme de Voronoi : limité aux antennes actives

---

## 🔧 Troubleshooting

### Erreur "Zone not found"

```bash
# Vérifier que la zone existe
curl http://localhost:8080/api/zones/1
```

### Optimisation lente

- Réduire `antennas_count`
- Utiliser `algorithm: "greedy"` au lieu de `kmeans`
- Vérifier les index PostGIS

### Signal toujours "poor"

- Augmenter `coverage_radius` des antennes
- Vérifier que des antennes existent dans la zone

---

## 📚 Ressources

- [Drogon Framework](https://github.com/drogonframework/drogon)
- [PostGIS Documentation](https://postgis.net/docs/)
- [Leaflet.js](https://leafletjs.com/)
- [RF Propagation Models](https://en.wikipedia.org/wiki/Radio_propagation_model)

---

## 📄 Licence

MIT License - voir fichier `LICENSE`

---

## 👥 Contributeurs

- Développement initial : [Votre nom]
- Algorithmes d'optimisation : [Équipe IA]

---

**Version:** 3.0.0  
**Dernière mise à jour:** 29 novembre 2024
