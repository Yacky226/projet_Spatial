Collecting workspace information# 📡 API Optimisation Couverture Réseau 4G/5G (Maroc)

## 📋 Vue d'ensemble

API REST haute performance développée en **C++17** avec le framework **Drogon** pour la gestion et l'optimisation des réseaux de télécommunications au Maroc. L'application utilise **PostgreSQL avec PostGIS** pour le traitement avancé des données géospatiales.

## 🏗️ Architecture

```
antennes-5g/
├── config/
│   └── config.json              # Configuration de la base de données
├── src/
│   ├── main.cpp                 # Point d'entrée de l'application
│   ├── controllers/             # Contrôleurs REST (endpoints HTTP)
│   │   ├── AntenneController    # Gestion des antennes
│   │   ├── OperatorController   # Gestion des opérateurs
│   │   ├── ZoneController       # Gestion des zones géographiques
│   │   ├── ObstacleController   # Gestion des obstacles
│   │   ├── AntennaZoneController    # Relations antennes-zones
│   │   └── ZoneObstacleController   # Relations zones-obstacles
│   ├── services/                # Logique métier et requêtes SQL
│   │   ├── AntenneService
│   │   ├── OperatorService
│   │   ├── ZoneService
│   │   ├── ObstacleService
│   │   ├── AntennaZoneService
│   │   └── ZoneObstacleService
│   └── models/                  # Structures de données
│       ├── Antenne.h
│       ├── Operator.h
│       ├── Zone.h
│       └── Obstacle.h
├── tests/
│   └── postman_collection.json  # Collection de tests Postman
├── scripts/
│   └── init.sql                 # Script d'initialisation de la DB
├── CMakeLists.txt               # Configuration de compilation
├── Dockerfile                   # Image Docker Ubuntu + Drogon
├── docker-compose.yml           # Orchestration des services
├── setup.bat                    # Script de démarrage Windows (Batch)
├── setup.ps1                    # Script de démarrage Windows (PowerShell)
└── quick-start.bat              # Démarrage rapide
```

## 🛠 Stack Technique

| Composant             | Technologie                           |
| --------------------- | ------------------------------------- |
| **Langage**           | C++17                                 |
| **Framework Web**     | Drogon (Asynchrone, Non-blocking I/O) |
| **Base de données**   | PostgreSQL 14+                        |
| **Extension SIG**     | PostGIS 3.x                           |
| **Conteneurisation**  | Docker & Docker Compose               |
| **Build System**      | CMake 3.14+                           |
| **Format de données** | JSON, GeoJSON (RFC 7946)              |

## 📡 Fonctionnalités Complètes

### 1. 📶 Gestion des Antennes (`/api/antennes`)

#### Opérations CRUD

| Méthode  | Endpoint             | Description                      |
| -------- | -------------------- | -------------------------------- |
| `GET`    | `/api/antennes`      | Liste toutes les antennes        |
| `GET`    | `/api/antennes/{id}` | Détails d'une antenne spécifique |
| `POST`   | `/api/antennes`      | Créer une nouvelle antenne       |
| `PUT`    | `/api/antennes/{id}` | Mettre à jour une antenne        |
| `DELETE` | `/api/antennes/{id}` | Supprimer une antenne            |

#### Fonctionnalités Géospatiales

| Méthode | Endpoint                                                   | Description                                  |
| ------- | ---------------------------------------------------------- | -------------------------------------------- |
| `GET`   | `/api/antennes/geojson`                                    | Export GeoJSON (compatible Leaflet/MapBox)   |
| `GET`   | `/api/antennes/search?lat={lat}&lon={lon}&radius={meters}` | Recherche dans un rayon (PostGIS ST_DWithin) |

#### Modèle de Données

```json
{
  "id": 1,
  "coverage_radius": 5000.0,
  "status": "active",
  "technology": "5G",
  "installation_date": "2024-01-15",
  "operator_id": 1,
  "latitude": 33.5731,
  "longitude": -7.5898
}
```

**Types Enum:**

- `status`: `active`, `inactive`, `maintenance`
- `technology`: `2G`, `3G`, `4G`, `5G`

### 2. 🏢 Gestion des Opérateurs (`/api/operators`)

| Méthode  | Endpoint              | Description                |
| -------- | --------------------- | -------------------------- |
| `GET`    | `/api/operators`      | Liste tous les opérateurs  |
| `GET`    | `/api/operators/{id}` | Détails d'un opérateur     |
| `POST`   | `/api/operators`      | Créer un opérateur         |
| `PUT`    | `/api/operators/{id}` | Mettre à jour un opérateur |
| `DELETE` | `/api/operators/{id}` | Supprimer un opérateur     |

#### Modèle de Données

```json
{
  "id": 1,
  "name": "Maroc Telecom"
}
```

**Exemples d'opérateurs:**

- Maroc Telecom
- Orange Maroc
- Inwi

### 3. 🗺️ Gestion des Zones (`/api/zones`)

#### Opérations CRUD

| Méthode  | Endpoint             | Description              |
| -------- | -------------------- | ------------------------ |
| `GET`    | `/api/zones`         | Liste toutes les zones   |
| `GET`    | `/api/zones/{id}`    | Détails d'une zone       |
| `POST`   | `/api/zones`         | Créer une zone           |
| `PUT`    | `/api/zones/{id}`    | Mettre à jour une zone   |
| `DELETE` | `/api/zones/{id}`    | Supprimer une zone       |
| `GET`    | `/api/zones/geojson` | Export GeoJSON des zones |

#### Modèle de Données

```json
{
  "id": 1,
  "name": "Casablanca Centre",
  "type": "country",
  "density": 1500.0,
  "wkt": "POLYGON((-7.6 33.57, -7.58 33.57, -7.58 33.59, -7.6 33.59, -7.6 33.57))",
  "parent_id": 0
}
```

**Types de zones:**

- `country`: Pays
- `region`: Région administrative
- `province`: Province
- `coverage`: Zone de couverture
- `white_zone`: Zone blanche (sans couverture)

**Hiérarchie:** Les zones peuvent avoir des relations parent-enfant (ex: Région → Province).

### 4. 🏔️ Gestion des Obstacles (`/api/obstacles`)

| Méthode  | Endpoint                 | Description                  |
| -------- | ------------------------ | ---------------------------- |
| `GET`    | `/api/obstacles`         | Liste tous les obstacles     |
| `GET`    | `/api/obstacles/{id}`    | Détails d'un obstacle        |
| `POST`   | `/api/obstacles`         | Créer un obstacle            |
| `PUT`    | `/api/obstacles/{id}`    | Mettre à jour un obstacle    |
| `DELETE` | `/api/obstacles/{id}`    | Supprimer un obstacle        |
| `GET`    | `/api/obstacles/geojson` | Export GeoJSON des obstacles |

#### Modèle de Données

```json
{
  "id": 1,
  "type": "batiment",
  "geom_type": "POLYGON",
  "wkt": "POLYGON((-7.59 33.58, -7.588 33.58, -7.588 33.582, -7.59 33.582, -7.59 33.58))"
}
```

**Types d'obstacles:**

- `batiment`: Bâtiments
- `foret`: Forêts
- `montagne`: Montagnes
- `eau`: Plans d'eau

**Géométries supportées:**

- `POINT`: Points isolés
- `LINESTRING`: Lignes (routes, rivières)
- `POLYGON`: Surfaces (bâtiments, zones boisées)

### 5. 🔗 Relations Antennes-Zones (`/antenna-zone/*`)

| Méthode | Endpoint                      | Description                     |
| ------- | ----------------------------- | ------------------------------- |
| `POST`  | `/antenna-zone/link`          | Lier une antenne à une zone     |
| `POST`  | `/antenna-zone/unlink`        | Délier une antenne d'une zone   |
| `GET`   | `/antenna/{antenna_id}/zones` | Zones couvertes par une antenne |
| `GET`   | `/zone/{zone_id}/antennas`    | Antennes dans une zone          |
| `GET`   | `/antenna-zone/all`           | Toutes les relations            |

#### Exemples de Requêtes

**Créer un lien:**

```json
POST /antenna-zone/link
{
  "antenna_id": 1,
  "zone_id": 5
}
```

**Réponse:**

```json
[1, 3, 5, 12] // Liste des IDs de zones
```

### 6. 🚧 Relations Zones-Obstacles (`/zone-obstacle/*`)

| Méthode | Endpoint                        | Description                     |
| ------- | ------------------------------- | ------------------------------- |
| `POST`  | `/zone-obstacle/link`           | Lier une zone à un obstacle     |
| `POST`  | `/zone-obstacle/unlink`         | Délier une zone d'un obstacle   |
| `GET`   | `/zone/{zone_id}/obstacles`     | Obstacles dans une zone         |
| `GET`   | `/obstacle/{obstacle_id}/zones` | Zones affectées par un obstacle |
| `GET`   | `/zone-obstacle/all`            | Toutes les relations            |

## 🗄️ Schéma de Base de Données

### Tables Principales

```sql
-- Opérateurs télécoms
CREATE TABLE operator (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE
);

-- Antennes (avec géométrie PostGIS)
CREATE TABLE antenna (
    id SERIAL PRIMARY KEY,
    coverage_radius FLOAT,
    status antenna_status NOT NULL,
    technology technology_type NOT NULL,
    installation_date DATE,
    operator_id INTEGER REFERENCES operator(id),
    geom GEOMETRY(Point, 4326) NOT NULL
);

-- Zones géographiques (polygones)
CREATE TABLE zone (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE,
    type zone_type NOT NULL,
    density DOUBLE PRECISION,
    parent_id INTEGER REFERENCES zone(id),
    geom GEOMETRY(Polygon, 4326) NOT NULL
);

-- Obstacles (géométries multiples)
CREATE TABLE obstacle (
    id SERIAL PRIMARY KEY,
    type VARCHAR(50) NOT NULL,
    geom_type VARCHAR(20),
    geom GEOMETRY NOT NULL
);

-- Table de liaison Antennes-Zones
CREATE TABLE antenna_zone (
    antenna_id INTEGER REFERENCES antenna(id) ON DELETE CASCADE,
    zone_id INTEGER REFERENCES zone(id) ON DELETE CASCADE,
    PRIMARY KEY (antenna_id, zone_id)
);

-- Table de liaison Zones-Obstacles
CREATE TABLE zone_obstacle (
    zone_id INTEGER REFERENCES zone(id) ON DELETE CASCADE,
    obstacle_id INTEGER REFERENCES obstacle(id) ON DELETE CASCADE,
    PRIMARY KEY (zone_id, obstacle_id)
);
```

### Index Géospatiaux

```sql
CREATE INDEX idx_antenna_geom ON antenna USING GIST (geom);
CREATE INDEX idx_zone_geom ON zone USING GIST (geom);
CREATE INDEX idx_obstacle_geom ON obstacle USING GIST (geom);
```

## 🚀 Installation et Démarrage

### Prérequis

- **Docker Desktop** (Windows/Mac/Linux)
- **PostgreSQL 14+** avec **PostGIS 3.x**
- **CMake 3.14+** (pour compilation locale)
- **Git** (pour cloner le dépôt)

### Configuration PostgreSQL

1. Modifiez [`C:\Program Files\PostgreSQL\14\data\postgresql.conf`](C:\Program Files\PostgreSQL\14\data\postgresql.conf):

```ini
listen_addresses = '*'
```

2. Modifiez [`C:\Program Files\PostgreSQL\14\data\pg_hba.conf`](C:\Program Files\PostgreSQL\14\data\pg_hba.conf):

```
host    all    all    0.0.0.0/0    trust
```

3. Redémarrez PostgreSQL dans `services.msc`.

### Démarrage Rapide

#### Option 1: Script Batch (Windows)

```bat
quick-start.bat
```

#### Option 2: Script PowerShell

```powershell
.\setup.ps1
```

#### Option 3: Docker Compose

```bash
docker-compose up -d --build
```

### Vérification

Testez le démarrage:

```bash
curl http://localhost:8080/health
```

Réponse attendue:

```json
{
  "status": "ok",
  "database": "connected",
  "postgis": "3.x"
}
```

## 📖 Exemples d'Utilisation

### 1. Créer un Opérateur

```bash
curl -X POST http://localhost:8080/api/operators \
  -H "Content-Type: application/json" \
  -d '{"name": "Maroc Telecom"}'
```

### 2. Créer une Antenne 5G

```bash
curl -X POST http://localhost:8080/api/antennes \
  -H "Content-Type: application/json" \
  -d '{
    "coverage_radius": 5000,
    "status": "active",
    "technology": "5G",
    "installation_date": "2024-01-15",
    "operator_id": 1,
    "latitude": 33.5731,
    "longitude": -7.5898
  }'
```

### 3. Rechercher les Antennes dans un Rayon

```bash
# Antennes dans un rayon de 10km autour de Casablanca
curl "http://localhost:8080/api/antennes/search?lat=33.5731&lon=-7.5898&radius=10000"
```

### 4. Créer une Zone

```bash
curl -X POST http://localhost:8080/api/zones \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Casablanca Centre",
    "type": "coverage",
    "density": 1500.0,
    "wkt": "POLYGON((-7.6 33.57, -7.58 33.57, -7.58 33.59, -7.6 33.59, -7.6 33.57))",
    "parent_id": 0
  }'
```

### 5. Lier une Antenne à une Zone

```bash
curl -X POST http://localhost:8080/antenna-zone/link \
  -H "Content-Type: application/json" \
  -d '{"antenna_id": 1, "zone_id": 1}'
```

### 6. Export GeoJSON pour Leaflet

```bash
curl http://localhost:8080/api/antennes/geojson > antennes.geojson
```

## 🧪 Tests avec Postman

Importez la collection de tests:

```bash
tests/postman_collection.json
```

**Tests disponibles:**

- ✅ Health Check
- ✅ Database Connection Test
- ✅ CRUD Antennes
- ✅ CRUD Opérateurs
- ✅ CRUD Zones
- ✅ CRUD Obstacles
- ✅ Relations Antennes-Zones
- ✅ Relations Zones-Obstacles
- ✅ Recherche Géospatiale
- ✅ Export GeoJSON

## 🐛 Dépannage

### Problème: "Connection refused"

**Solution:** Vérifiez que PostgreSQL écoute sur `0.0.0.0`:

```bash
psql -U yacouba -h localhost -c "SELECT 1"
```

### Problème: "Address already in use :8080"

**Solution:** Changez le port dans docker-compose.yml:

```yaml
ports:
  - "8081:8080" # Utiliser le port 8081 au lieu de 8080
```

### Problème: Logs vides

**Solution:**

```bash
docker-compose logs -f api_cpp
```

## 📊 Commandes Utiles

### Docker

```bash
# Voir les services en cours
docker-compose ps

# Voir les logs en temps réel
docker-compose logs -f api_cpp

# Redémarrer l'API
docker-compose restart api_cpp

# Arrêter les services
docker-compose down

# Supprimer les volumes (réinitialisation complète)
docker-compose down -v

# Rebuild complet
docker-compose up -d --build
```

### PostgreSQL

```bash
# Se connecter à la base
psql -U yacouba -h localhost -d antennes_5g

# Lister les tables
\dt

# Vérifier PostGIS
SELECT PostGIS_version();

# Compter les antennes
SELECT COUNT(*) FROM antenna;
```

## 📚 Documentation Technique

- **Drogon Framework:** [https://github.com/drogonframework/drogon](https://github.com/drogonframework/drogon)
- **PostGIS:** [https://postgis.net/documentation/](https://postgis.net/documentation/)
- **GeoJSON Spec:** [https://tools.ietf.org/html/rfc7946](https://tools.ietf.org/html/rfc7946)
