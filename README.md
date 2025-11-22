Markdown

# 📡 API Optimisation Couverture Réseau 4G/5G (Maroc)

Ce projet est une API haute performance développée en **C++ (Drogon)** destinée à simuler et optimiser le placement des antennes réseaux au Maroc. Elle utilise **PostgreSQL avec PostGIS** pour le traitement des données géospatiales.

L'architecture repose sur un conteneur Docker pour l'API C++, communiquant avec une base de données hébergée sur la machine hôte.

## 🛠 Technologies

- **Langage :** C++17
- **Framework Web :** Drogon (Non-blocking I/O)
- **Base de données :** PostgreSQL 14+
- **Extension SIG :** PostGIS 3.x
- **Conteneurisation :** Docker & Docker Compose
- **Build System :** CMake

---

## 📂 Architecture du Projet

```text
antennes-5g/
├── config/
│   └── config.json         # Config DB (host.docker.internal)
├── src/
│   ├── controllers/        # Endpoints HTTP (API REST)
│   ├── models/             # Structures de données (Structs C++)
│   ├── services/           # Logique métier & Requêtes SQL/PostGIS
│   └── main.cc             # Point d'entrée
├── CMakeLists.txt          # Configuration de compilation
├── Dockerfile              # Environnement Ubuntu + Drogon
├── docker-compose.yml      # Orchestration
└── README.md
⚙️ Prérequis & Configuration (CRITIQUE)
Puisque la base de données est sur Windows (Hôte) et l'API dans Docker, une configuration réseau spécifique est nécessaire.

1. Configuration PostgreSQL (Windows)
Fichiers situés dans C:\Program Files\PostgreSQL\14\data\ :

postgresql.conf :

Ini, TOML

listen_addresses = '*'
pg_hba.conf (Ajouter à la fin) :

Plaintext

# Autoriser Docker à se connecter (scram-sha-256 ou trust)
host    all             all             0.0.0.0/0            trust
(N'oubliez pas de redémarrer le service PostgreSQL via services.msc après modification).

2. Pare-feu Windows
Une règle de trafic entrant TCP sur le port 5432 doit être créée pour autoriser la connexion venant du conteneur.

3. Initialisation de la Base de Données
Exécutez ce script SQL dans votre base antennes_5g :

SQL

CREATE EXTENSION IF NOT EXISTS postgis;

CREATE TYPE technology_type AS ENUM ('2G', '3G', '4G', '5G');

CREATE TABLE IF NOT EXISTS operator (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS antenna (
    id SERIAL PRIMARY KEY,
    coverage_radius FLOAT,
    technology technology_type NOT NULL,
    geom GEOMETRY(Point, 4326) NOT NULL, -- Lat/Lon WGS84
    operator_id INTEGER NOT NULL REFERENCES operator(id)
);

INSERT INTO operator (name) VALUES ('Maroc Telecom'), ('Orange'), ('Inwi') ON CONFLICT DO NOTHING;
🚀 Installation et Lancement
Construire et lancer le conteneur :

Bash

docker-compose up --build
Vérifier le démarrage : Le terminal doit afficher :

Plaintext

HTTP server listening on 0.0.0.0:8080
🔌 Documentation de l'API
1. Lister les antennes
Récupère toutes les antennes avec leurs coordonnées (converties depuis PostGIS).

URL : GET http://localhost:8080/api/antennes

Réponse (200 OK) :

JSON

[
    {
        "id": 1,
        "technology": "5G",
        "coverage_radius": 10.0,
        "latitude": 33.5731,
        "longitude": -7.5898,
        "operator_id": 1
    }
]
2. Créer une antenne
Insère une nouvelle antenne en convertissant automatiquement Lat/Lon vers GEOMETRY(Point, 4326).

URL : POST http://localhost:8080/api/antennes

Body (JSON) :

JSON

{
    "technology": "5G",
    "coverage_radius": 5.5,
    "operator_id": 1,
    "latitude": 33.5890,
    "longitude": -7.6100
}
Réponse (201 Created) :

Plaintext

Antenne created
🐛 Troubleshooting (Problèmes fréquents)
Problème : L'application reste bloquée sur "Démarrage..." ou erreur "Connection refused".

Cause : Docker n'arrive pas à contacter PostgreSQL sur l'hôte.

Solution :

Vérifiez que le service PostgreSQL est démarré.

Vérifiez que listen_addresses = '*' est bien configuré.

Vérifiez que le Pare-feu Windows autorise le port 5432.

Testez la connectivité depuis le conteneur :

Bash

docker exec -it api_antennes_cpp bash
curl -v telnet://host.docker.internal:5432
Problème : Erreur "relation antenna does not exist".

```
