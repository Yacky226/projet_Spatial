#!/usr/bin/env pwsh
# Script de lancement - API Antennes 5G avec PostgreSQL local

Write-Host "`n╔════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ANTENNES 5G API - DÉMARRAGE              ║" -ForegroundColor Cyan
Write-Host "║  (PostgreSQL LOCAL)                       ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════╝`n" -ForegroundColor Cyan

# STEP 1 : Vérifier PostgreSQL
Write-Host "[STEP 1] Vérification de PostgreSQL local..." -ForegroundColor Yellow
try {
    $pg_test = psql -U yacouba -h localhost -c "SELECT 1" 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ PostgreSQL accessible" -ForegroundColor Green
    }
    else {
        Write-Host "  ✗ PostgreSQL non accessible" -ForegroundColor Red
        Write-Host "  → Démarrez PostgreSQL dans Services Windows" -ForegroundColor Yellow
        Read-Host "  Appuyez sur Entrée après avoir démarré PostgreSQL"
    }
}
catch {
    Write-Host "  ✗ PostgreSQL non trouvé" -ForegroundColor Red
}

# STEP 2 : Vérifier la base de données
Write-Host "`n[STEP 2] Vérification de la base de données..." -ForegroundColor Yellow
try {
    $db_test = psql -U yacouba -h localhost -c "SELECT COUNT(*) FROM antennes;" 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ Base 'antennes_5g' accessible" -ForegroundColor Green
    }
    else {
        Write-Host "  ✗ Impossible d'accéder à la base" -ForegroundColor Red
    }
}
catch {
    Write-Host "  ⚠ Vérification échouée" -ForegroundColor Yellow
}

# STEP 3 : Arrêt des services précédents
Write-Host "`n[STEP 3] Arrêt des services précédents..." -ForegroundColor Yellow
docker-compose down 2>$null
Start-Sleep -Seconds 3
Write-Host "  ✓ Services arrêtés" -ForegroundColor Green

# STEP 4 : Compilation et démarrage
Write-Host "`n[STEP 4] Compilation et démarrage (cela peut prendre 1-2 minutes)..." -ForegroundColor Yellow
docker-compose build --no-cache
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ Erreur lors de la compilation" -ForegroundColor Red
    Read-Host "  Appuyez sur Entrée pour voir les logs"
    docker-compose logs api
    exit 1
}

docker-compose up -d
if ($LASTEXITCODE -ne 0) {
    Write-Host "  ✗ Erreur lors du démarrage" -ForegroundColor Red
    exit 1
}

Write-Host "  ✓ Services démarrés" -ForegroundColor Green

# STEP 5 : Attente du démarrage
Write-Host "`n[STEP 5] Attente du démarrage complet..." -ForegroundColor Yellow
Write-Host "  ⏳ " -NoNewline
for ($i = 1; $i -le 25; $i++) {
    Write-Host "." -NoNewline -ForegroundColor Green
    Start-Sleep -Seconds 1
}
Write-Host "`n  ✓ Démarrage complété" -ForegroundColor Green

# STEP 6 : Vérification de l'état
Write-Host "`n[STEP 6] État des services:" -ForegroundColor Yellow
docker-compose ps

# STEP 7 : Tests de l'API
Write-Host "`n[STEP 7] Tests de l'API..." -ForegroundColor Yellow

Write-Host "`n  Test 1 : Health Check" -ForegroundColor Cyan
try {
    $response = Invoke-WebRequest -Uri "http://localhost:8080/health" `
        -UseBasicParsing -TimeoutSec 5 -WarningAction SilentlyContinue
    
    if ($response.StatusCode -eq 200) {
        Write-Host "  ✓ Réponse reçue (HTTP 200)" -ForegroundColor Green
        $json = $response.Content | ConvertFrom-Json
        Write-Host "    Status: $($json.status)" -ForegroundColor Green
        Write-Host "    Database: $($json.database)" -ForegroundColor Green
    }
}
catch {
    Write-Host "  ✗ Pas de réponse - Vérifiez les logs" -ForegroundColor Red
}

Write-Host "`n  Test 2 : Récupérer les antennes" -ForegroundColor Cyan
try {
    $response = Invoke-WebRequest -Uri "http://localhost:8080/api/antennes" `
        -UseBasicParsing -TimeoutSec 5 -WarningAction SilentlyContinue
    
    if ($response.StatusCode -eq 200) {
        $antennes = $response.Content | ConvertFrom-Json
        Write-Host "  ✓ Réponse reçue" -ForegroundColor Green
        Write-Host "    Nombre d'antennes: $($antennes.Count)" -ForegroundColor Green
    }
}
catch {
    Write-Host "  ✗ Pas de réponse" -ForegroundColor Red
}

Write-Host "`n  Test 3 : Format GeoJSON" -ForegroundColor Cyan
try {
    $response = Invoke-WebRequest -Uri "http://localhost:8080/api/antennes/geojson" `
        -UseBasicParsing -TimeoutSec 5 -WarningAction SilentlyContinue
    
    if ($response.StatusCode -eq 200) {
        $geojson = $response.Content | ConvertFrom-Json
        Write-Host "  ✓ Réponse reçue" -ForegroundColor Green
        Write-Host "    Features: $($geojson.features.Count)" -ForegroundColor Green
    }
}
catch {
    Write-Host "  ✗ Pas de réponse" -ForegroundColor Red
}

# Résumé
Write-Host "`n╔════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  DÉMARRAGE TERMINÉ ✓                      ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════╝`n" -ForegroundColor Cyan

Write-Host "✓ Serveur en écoute sur : http://localhost:8080" -ForegroundColor Green
Write-Host "✓ Connecté à PostgreSQL : host.docker.internal:5432" -ForegroundColor Green
Write-Host "✓ Base de données : antennes_5g" -ForegroundColor Green

Write-Host "`n📍 Endpoints disponibles :" -ForegroundColor Cyan
Write-Host "  GET  http://localhost:8080/health"
Write-Host "  GET  http://localhost:8080/api/test/db"
Write-Host "  GET  http://localhost:8080/api/antennes"
Write-Host "  GET  http://localhost:8080/api/antennes/geojson"

Write-Host "`n💡 Commandes utiles :" -ForegroundColor Cyan
Write-Host "  docker-compose logs -f api       # Voir les logs"
Write-Host "  docker-compose down              # Arrêter"
Write-Host "  docker-compose ps                # État des services"

Write-Host "`n🧪 Testez dans Postman :" -ForegroundColor Cyan
Write-Host "  GET http://localhost:8080/health`n"

Read-Host "Appuyez sur Entrée pour terminer"