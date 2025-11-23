# API Antennes 5G - Documentation Complète

## 📋 Table des matières

1. Vue d'ensemble
2. Architecture
3. Modèles de données
4. Endpoints API
5. Pagination
6. Validation
7. Gestion des erreurs
8. Installation
9. Exemples d'utilisation

---

## 🎯 Vue d'ensemble

API REST pour la gestion et la visualisation d'antennes 5G avec support PostGIS pour les fonctionnalités géospatiales.

### Fonctionnalités principales

- ✅ CRUD complet pour antennes, obstacles, zones et opérateurs
- ✅ Recherche géographique (rayon, bounding box)
- ✅ Export GeoJSON pour intégration Leaflet/Mapbox
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
│   └── ZoneObstacleController.cc
│
├── services/          # Logique métier et accès DB
│   ├── AntenneService.cc
│   ├── ObstacleService.cc
│   ├── ZoneService.cc
│   ├── OperatorService.cc
│   ├── AntennaZoneService.cc
│   └── ZoneObstacleService.cc
│
├── models/           # Structures de données
│   ├── Antenne.h
│   ├── Obstacle.h
│   ├── Zone.h
│   └── Operator.h
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
  "wkt": "POLYGON((2.33 48.86, 2.34 48.86, 2.34 48.87, 2.33 48.87, 2.33 48.86))"
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
- PostgreSQL 14+ avec PostGIS
- C++17
- Drogon Framework

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
```

### Build

```bash
# Cloner le projet
git clone <repo-url>
cd antennes-5g

# Build Docker
docker build -t antennes-5g-api .

# Lancer l'API
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
    "client_encoding": "UTF8"
  }
}
```

---

## 💡 Exemples d'utilisation

### Créer une antenne avec cURL

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

### Recherche dans un rayon

```bash
curl "http://localhost:8080/api/antennes/search/radius?lat=48.8566&lon=2.3522&radius=10000"
```

### Export GeoJSON pour Leaflet

```javascript
// JavaScript avec Leaflet
fetch("http://localhost:8080/api/antennes/geojson")
  .then((res) => res.json())
  .then((data) => {
    L.geoJSON(data, {
      pointToLayer: (feature, latlng) => {
        return L.circle(latlng, {
          radius: feature.properties.coverage_radius,
          color: feature.properties.status === "active" ? "green" : "red",
        });
      },
    }).addTo(map);
  });
```

### Pagination

```bash
# Page 2, 50 éléments par page
curl "http://localhost:8080/api/antennes?page=2&pageSize=50"
```

---

## 📝 Notes techniques

### PostGIS

L'API utilise PostGIS pour:

- Calcul de distances géographiques (ST_Distance)
- Recherche dans un rayon (ST_DWithin)
- Recherche dans une bbox (ST_MakeEnvelope)
- Calcul d'aires (ST_Area)
- Export GeoJSON (ST_AsGeoJSON)

### Performance

- Index spatial sur colonnes géométriques
- Pagination obligatoire au-delà de 100 éléments
- Cache des requêtes fréquentes recommandé
