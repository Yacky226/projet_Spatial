-- ========================================
-- Initialisation de la base de données
-- Antennes 5G - PostGIS
-- ========================================

-- Créer l'extension PostGIS
CREATE EXTENSION IF NOT EXISTS postgis;
CREATE EXTENSION IF NOT EXISTS postgis_topology;

-- ========================================
-- TABLE : ANTENNES
-- ========================================
CREATE TABLE IF NOT EXISTS antennes (
    id SERIAL PRIMARY KEY,
    latitude DOUBLE PRECISION NOT NULL,
    longitude DOUBLE PRECISION NOT NULL,
    geom GEOGRAPHY(POINT, 4326) NOT NULL,
    type VARCHAR(20) NOT NULL CHECK (type IN ('4G', '5G', '4G/5G')),
    puissance INTEGER NOT NULL, -- en watts
    portee INTEGER NOT NULL, -- en mètres
    statut VARCHAR(20) NOT NULL DEFAULT 'active' CHECK (statut IN ('active', 'inactive', 'maintenance')),
    date_installation DATE NOT NULL,
    date_creation TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT antennes_geom_idx UNIQUE (geom)
);

-- Créer l'index géospatial
CREATE INDEX idx_antennes_geom ON antennes USING GIST (geom);
CREATE INDEX idx_antennes_type ON antennes(type);
CREATE INDEX idx_antennes_statut ON antennes(statut);

-- ========================================
-- TABLE : ZONES
-- ========================================
CREATE TABLE IF NOT EXISTS zones (
    id SERIAL PRIMARY KEY,
    nom VARCHAR(255) NOT NULL UNIQUE,
    description TEXT,
    type_zone VARCHAR(50) NOT NULL, -- "urbaine", "rurale", "mixte"
    population INTEGER,
    geom GEOGRAPHY(POLYGON, 4326) NOT NULL,
    date_creation TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Créer l'index géospatial pour zones
CREATE INDEX idx_zones_geom ON zones USING GIST (geom);
CREATE INDEX idx_zones_type ON zones(type_zone);

-- ========================================
-- TABLE : COUVERTURE_ZONES
-- ========================================
CREATE TABLE IF NOT EXISTS couverture_zones (
    id SERIAL PRIMARY KEY,
    antenne_id INTEGER NOT NULL REFERENCES antennes(id) ON DELETE CASCADE,
    zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE CASCADE,
    pourcentage_couverture DECIMAL(5, 2) NOT NULL,
    qualite_signal VARCHAR(20) CHECK (qualite_signal IN ('excellent', 'bon', 'moyen', 'faible')),
    date_analyse TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT unique_antenne_zone UNIQUE (antenne_id, zone_id)
);

-- Créer les index
CREATE INDEX idx_couverture_antenne ON couverture_zones(antenne_id);
CREATE INDEX idx_couverture_zone ON couverture_zones(zone_id);

-- ========================================
-- TABLE : INTERFERANCES
-- ========================================
CREATE TABLE IF NOT EXISTS interferences (
    id SERIAL PRIMARY KEY,
    antenne_source_id INTEGER NOT NULL REFERENCES antennes(id) ON DELETE CASCADE,
    antenne_cible_id INTEGER NOT NULL REFERENCES antennes(id) ON DELETE CASCADE,
    niveau_interference DECIMAL(5, 2) NOT NULL, -- en dB
    distance DECIMAL(10, 2) NOT NULL, -- en mètres
    date_mesure TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Créer les index
CREATE INDEX idx_interference_source ON interferences(antenne_source_id);
CREATE INDEX idx_interference_cible ON interferences(antenne_cible_id);

-- ========================================
-- TABLE : SCENARIOS_OPTIMISATION
-- ========================================
CREATE TABLE IF NOT EXISTS scenarios_optimisation (
    id SERIAL PRIMARY KEY,
    nom VARCHAR(255) NOT NULL UNIQUE,
    description TEXT,
    parametres JSONB NOT NULL, -- Paramètres d'optimisation en JSON
    resultat_couverture DECIMAL(5, 2), -- % de couverture global
    cout_estimé DECIMAL(15, 2), -- Coût des antennes
    statut VARCHAR(20) DEFAULT 'en_cours' CHECK (statut IN ('en_cours', 'complété', 'annulé')),
    date_creation TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    date_finalisation TIMESTAMP
);

CREATE INDEX idx_scenarios_statut ON scenarios_optimisation(statut);

-- ========================================
-- DONNÉES D'EXEMPLE
-- ========================================

-- Insérer des antennes exemple
INSERT INTO antennes (latitude, longitude, geom, type, puissance, portee, statut, date_installation)
VALUES 
    (33.5731, -7.5898, ST_GeogFromText('SRID=4326;POINT(-7.5898 33.5731)'), '5G', 100, 5000, 'active', '2023-01-15'),
    (33.5735, -7.5900, ST_GeogFromText('SRID=4326;POINT(-7.5900 33.5735)'), '4G', 80, 4500, 'active', '2023-02-20'),
    (33.5740, -7.5895, ST_GeogFromText('SRID=4326;POINT(-7.5895 33.5740)'), '4G/5G', 120, 6000, 'maintenance', '2023-03-10')
ON CONFLICT DO NOTHING;

-- Insérer des zones exemple
INSERT INTO zones (nom, description, type_zone, population, geom)
VALUES 
    ('Casablanca Centre', 'Zone urbaine centrale', 'urbaine', 500000, 
     ST_GeogFromText('SRID=4326;POLYGON((-7.6 33.57, -7.58 33.57, -7.58 33.59, -7.6 33.59, -7.6 33.57))')),
    ('Aïn Chock', 'Zone urbaine périphérique', 'urbaine', 250000,
     ST_GeogFromText('SRID=4326;POLYGON((-7.65 33.50, -7.60 33.50, -7.60 33.55, -7.65 33.55, -7.65 33.50))'))
ON CONFLICT DO NOTHING;

-- Insérer des couvertures exemple
INSERT INTO couverture_zones (antenne_id, zone_id, pourcentage_couverture, qualite_signal)
VALUES 
    (1, 1, 95.5, 'excellent'),
    (1, 2, 75.2, 'bon'),
    (2, 1, 88.3, 'bon'),
    (3, 2, 92.1, 'excellent')
ON CONFLICT DO NOTHING;

-- ========================================
-- VUE : Résumé des couvertures par zone
-- ========================================
CREATE OR REPLACE VIEW vue_couverture_par_zone AS
SELECT 
    z.id,
    z.nom as zone_nom,
    COUNT(DISTINCT a.id) as nombre_antennes,
    AVG(c.pourcentage_couverture) as couverture_moyenne,
    MAX(c.pourcentage_couverture) as couverture_max,
    z.population
FROM zones z
LEFT JOIN couverture_zones c ON z.id = c.zone_id
LEFT JOIN antennes a ON c.antenne_id = a.id
GROUP BY z.id, z.nom, z.population;

-- ========================================
-- FONCTION : Calculer la distance entre deux points
-- ========================================
CREATE OR REPLACE FUNCTION distance_antennes(
    ant1_id INTEGER,
    ant2_id INTEGER
) RETURNS DECIMAL AS $$
DECLARE
    dist DECIMAL;
BEGIN
    SELECT ST_Distance(a1.geom, a2.geom) / 1000.0 INTO dist
    FROM antennes a1, antennes a2
    WHERE a1.id = ant1_id AND a2.id = ant2_id;
    RETURN ROUND(dist::NUMERIC, 2);
END;
$$ LANGUAGE plpgsql;

-- ========================================
-- FONCTION : Compter antennes dans une zone
-- ========================================
CREATE OR REPLACE FUNCTION compter_antennes_dans_zone(zone_id INTEGER)
RETURNS INTEGER AS $$
DECLARE
    count INTEGER;
BEGIN
    SELECT COUNT(*) INTO count
    FROM antennes a, zones z
    WHERE z.id = zone_id AND ST_Contains(z.geom, a.geom);
    RETURN count;
END;
$$ LANGUAGE plpgsql;

-- ========================================
-- Afficher les tables créées
-- ========================================
\echo '✓ Tables créées avec succès !'
\echo ''
\echo 'Tables disponibles :'
\dt
\echo ''
\echo 'Extensions PostGIS activées :'
\dx postgis*

-- ========================================
-- Fin de l'initialisation
-- ========================================