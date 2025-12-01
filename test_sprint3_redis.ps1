# Sprint 3 - Script de Tests d'Intégration Cache Redis

## Tests Automatiques

### Test 1: Santé Services Docker
```powershell
# Vérifier tous les conteneurs sont UP
docker ps

# Vérifier santé Redis
docker exec redis_cache redis-cli -a antennes5g_redis_pass ping
# Attendu: PONG

# Vérifier logs API (init Redis)
docker logs api_antennes_cpp | Select-String "Redis connected"
# Attendu: "✅ Redis connected: redis:6379"
```

### Test 2: Cache HIT/MISS Zones (Premier Endpoint)
```powershell
# === MISS Premier Appel ===
Write-Host "`n=== TEST ZONES: Premier appel (MISS attendu) ===" -ForegroundColor Yellow

$response1 = Invoke-WebRequest -Uri "http://localhost:8082/api/zones/type/commune/simplified?zoom=8" -Method GET -Headers @{"Accept"="application/json"} -UseBasicParsing

Write-Host "Status Code: $($response1.StatusCode)"
Write-Host "X-Cache Header: $($response1.Headers['X-Cache'])"
Write-Host "Content-Length: $($response1.Content.Length)"

# Vérifier logs API
docker logs --tail 50 api_antennes_cpp | Select-String "Cache MISS"

Start-Sleep -Seconds 2

# === HIT Deuxième Appel ===
Write-Host "`n=== TEST ZONES: Deuxième appel (HIT attendu) ===" -ForegroundColor Green

$response2 = Invoke-WebRequest -Uri "http://localhost:8082/api/zones/type/commune/simplified?zoom=8" -Method GET -Headers @{"Accept"="application/json"} -UseBasicParsing

Write-Host "Status Code: $($response2.StatusCode)"
Write-Host "X-Cache Header: $($response2.Headers['X-Cache'])"
Write-Host "Content-Length: $($response2.Content.Length)"

# Vérifier logs API
docker logs --tail 50 api_antennes_cpp | Select-String "Cache HIT"

# === Vérification Redis ===
Write-Host "`n=== Vérification clés Redis ===" -ForegroundColor Cyan
docker exec redis_cache redis-cli -a antennes5g_redis_pass KEYS "zones:*"
docker exec redis_cache redis-cli -a antennes5g_redis_pass TTL "zones:type:commune:zoom:8"
# Attendu: ~3600 secondes (1h)
```

### Test 3: Cache HIT/MISS Clusters (Deuxième Endpoint)
```powershell
# === Définir bbox de test ===
$bbox = "minLat=48.8&minLon=2.2&maxLat=48.9&maxLon=2.4"
$zoom = 12

# === MISS Premier Appel ===
Write-Host "`n=== TEST CLUSTERS: Premier appel (MISS attendu) ===" -ForegroundColor Yellow

$url1 = "http://localhost:8082/api/antennes/clustered?$bbox&zoom=$zoom"
$response1 = Invoke-WebRequest -Uri $url1 -Method GET -Headers @{"Accept"="application/geo+json"} -UseBasicParsing

Write-Host "Status Code: $($response1.StatusCode)"
Write-Host "X-Cache Header: $($response1.Headers['X-Cache'])"
Write-Host "Content-Type: $($response1.Headers['Content-Type'])"

# Vérifier logs
docker logs --tail 50 api_antennes_cpp | Select-String "Cache MISS.*clusters"

Start-Sleep -Seconds 2

# === HIT Deuxième Appel ===
Write-Host "`n=== TEST CLUSTERS: Deuxième appel (HIT attendu) ===" -ForegroundColor Green

$response2 = Invoke-WebRequest -Uri $url1 -Method GET -Headers @{"Accept"="application/geo+json"} -UseBasicParsing

Write-Host "Status Code: $($response2.StatusCode)"
Write-Host "X-Cache Header: $($response2.Headers['X-Cache'])"
Write-Host "Content-Type: $($response2.Headers['Content-Type'])"

# Vérifier logs
docker logs --tail 50 api_antennes_cpp | Select-String "Cache HIT.*clusters"

# === Vérification Redis ===
Write-Host "`n=== Vérification clés Redis clusters ===" -ForegroundColor Cyan
docker exec redis_cache redis-cli -a antennes5g_redis_pass KEYS "clusters:*"
docker exec redis_cache redis-cli -a antennes5g_redis_pass TTL "clusters:bbox:48.8:2.2:48.9:2.4:z:12"
# Attendu: ~120 secondes (2min)
```

### Test 4: Cache avec Filtres (Clés Différentes)
```powershell
Write-Host "`n=== TEST FILTRES: Différents filtres = différentes clés ===" -ForegroundColor Yellow

$bbox = "minLat=48.8&minLon=2.2&maxLat=48.9&maxLon=2.4"
$zoom = 12

# Appel avec status=active
$url1 = "http://localhost:8082/api/antennes/clustered?$bbox&zoom=$zoom&status=active"
Invoke-WebRequest -Uri $url1 -Method GET -UseBasicParsing | Out-Null

# Appel avec status=inactive
$url2 = "http://localhost:8082/api/antennes/clustered?$bbox&zoom=$zoom&status=inactive"
Invoke-WebRequest -Uri $url2 -Method GET -UseBasicParsing | Out-Null

# Vérifier 2 clés différentes créées
Write-Host "Clés clusters avec filtres:"
docker exec redis_cache redis-cli -a antennes5g_redis_pass KEYS "clusters:*:st:*"
# Attendu: 2 clés distinctes (une avec :st:active, une avec :st:inactive)
```

### Test 5: Invalidation Cache sur UPDATE
```powershell
Write-Host "`n=== TEST INVALIDATION: UPDATE zone ===" -ForegroundColor Yellow

# Créer entrée cache
$url = "http://localhost:8082/api/zones/type/commune/simplified?zoom=8"
Invoke-WebRequest -Uri $url -Method GET -UseBasicParsing | Out-Null
Write-Host "Cache créé (MISS)"

# Vérifier cache existe
docker exec redis_cache redis-cli -a antennes5g_redis_pass EXISTS "zones:type:commune:zoom:8"
# Attendu: 1

# Modifier une zone (adapter ID existant)
$zoneId = 1  # Adapter selon votre DB
$body = @{
    name = "Test Update Cache"
    type = "commune"
} | ConvertTo-Json

try {
    Invoke-WebRequest -Uri "http://localhost:8082/api/zones/$zoneId" -Method PUT -Body $body -ContentType "application/json" -UseBasicParsing | Out-Null
    Write-Host "Zone modifiée"
} catch {
    Write-Host "Erreur UPDATE (vérifier que zone $zoneId existe): $_"
}

# Vérifier cache invalidé
docker exec redis_cache redis-cli -a antennes5g_redis_pass EXISTS "zones:type:commune:zoom:8"
# Attendu: 0 (clé supprimée)

# Re-requête devrait être MISS
$response = Invoke-WebRequest -Uri $url -Method GET -UseBasicParsing
Write-Host "X-Cache après UPDATE: $($response.Headers['X-Cache'])"
# Attendu: MISS (cache a été invalidé)
```

### Test 6: Invalidation Cache sur DELETE
```powershell
Write-Host "`n=== TEST INVALIDATION: DELETE zone ===" -ForegroundColor Yellow

# Créer entrée cache
$url = "http://localhost:8082/api/zones/type/commune/simplified?zoom=8"
Invoke-WebRequest -Uri $url -Method GET -UseBasicParsing | Out-Null

# Compter clés avant delete
$countBefore = docker exec redis_cache redis-cli -a antennes5g_redis_pass KEYS "zones:*" | Measure-Object -Line
Write-Host "Clés zones avant DELETE: $($countBefore.Lines)"

# Supprimer une zone (adapter ID)
$zoneId = 999  # ID fictif pour test (adapter si nécessaire)
try {
    Invoke-WebRequest -Uri "http://localhost:8082/api/zones/$zoneId" -Method DELETE -UseBasicParsing | Out-Null
} catch {
    Write-Host "Zone $zoneId n'existe pas (normal si test)"
}

# Compter clés après delete (devrait être 0 car pattern "zones:*" invalidé)
$countAfter = docker exec redis_cache redis-cli -a antennes5g_redis_pass KEYS "zones:*" | Measure-Object -Line
Write-Host "Clés zones après DELETE: $($countAfter.Lines)"
# Attendu: 0 (toutes les clés zones:* supprimées)
```

### Test 7: Performance - Temps de Réponse Cache HIT vs MISS
```powershell
Write-Host "`n=== TEST PERFORMANCE: Mesure temps réponse ===" -ForegroundColor Cyan

$url = "http://localhost:8082/api/zones/type/commune/simplified?zoom=8"

# Mesurer MISS (première requête)
$sw = [System.Diagnostics.Stopwatch]::StartNew()
Invoke-WebRequest -Uri $url -Method GET -UseBasicParsing | Out-Null
$sw.Stop()
$timeMiss = $sw.ElapsedMilliseconds
Write-Host "Temps MISS (première requête): $timeMiss ms"

# Mesurer HIT (deuxième requête)
$sw = [System.Diagnostics.Stopwatch]::StartNew()
Invoke-WebRequest -Uri $url -Method GET -UseBasicParsing | Out-Null
$sw.Stop()
$timeHit = $sw.ElapsedMilliseconds
Write-Host "Temps HIT (cache): $timeHit ms"

# Calcul gain
$gain = [math]::Round((($timeMiss - $timeHit) / $timeMiss) * 100, 2)
Write-Host "Gain performance: -$gain%"
Write-Host "Objectif: Cache HIT < 100ms" -ForegroundColor $(if($timeHit -lt 100){"Green"}else{"Red"})
```

### Test 8: Ratio Cache HIT/MISS
```powershell
Write-Host "`n=== TEST RATIO: Calculer HIT/MISS ratio ===" -ForegroundColor Cyan

# Simuler utilisation (20 requêtes)
$url = "http://localhost:8082/api/zones/type/commune/simplified?zoom=8"
Write-Host "Simulation 20 requêtes..."
1..20 | ForEach-Object { 
    Invoke-WebRequest -Uri $url -Method GET -UseBasicParsing | Out-Null
    Start-Sleep -Milliseconds 100
}

# Extraire logs
$logs = docker logs api_antennes_cpp 2>&1 | Out-String
$hits = ([regex]::Matches($logs, "Cache HIT")).Count
$misses = ([regex]::Matches($logs, "Cache MISS")).Count
$total = $hits + $misses

if($total -gt 0) {
    $ratio = [math]::Round(($hits / $total) * 100, 2)
    Write-Host "HITs: $hits"
    Write-Host "MISSs: $misses"
    Write-Host "Ratio HIT: $ratio%"
    Write-Host "Objectif: >70% après warmup" -ForegroundColor $(if($ratio -gt 70){"Green"}else{"Yellow"})
} else {
    Write-Host "Pas de logs cache détectés" -ForegroundColor Red
}
```

### Test 9: Fallback Sans Redis
```powershell
Write-Host "`n=== TEST FALLBACK: API sans Redis ===" -ForegroundColor Yellow

# Arrêter Redis
Write-Host "Arrêt Redis..."
docker stop redis_cache

Start-Sleep -Seconds 3

# Tester API (doit fonctionner sans cache)
try {
    $response = Invoke-WebRequest -Uri "http://localhost:8082/api/zones/type/commune/simplified?zoom=8" -Method GET -UseBasicParsing
    Write-Host "Status Code: $($response.StatusCode) - API fonctionne sans Redis ✅" -ForegroundColor Green
} catch {
    Write-Host "Erreur API sans Redis ❌: $_" -ForegroundColor Red
}

# Vérifier logs fallback
docker logs --tail 20 api_antennes_cpp | Select-String "proceeding without cache"

# Redémarrer Redis
Write-Host "Redémarrage Redis..."
docker start redis_cache
Start-Sleep -Seconds 5

# Vérifier reconnexion (nécessite redémarrage API ou gestion reconnexion automatique)
Write-Host "Redis redémarré - API devrait utiliser cache à nouveau"
```

### Test 10: Monitoring Redis
```powershell
Write-Host "`n=== MONITORING REDIS ===" -ForegroundColor Cyan

# Stats Redis
Write-Host "`n--- Redis INFO Stats ---"
docker exec redis_cache redis-cli -a antennes5g_redis_pass INFO stats | Select-String -Pattern "keyspace_hits|keyspace_misses|total_commands_processed"

# Mémoire Redis
Write-Host "`n--- Redis Memory ---"
docker exec redis_cache redis-cli -a antennes5g_redis_pass INFO memory | Select-String -Pattern "used_memory_human|used_memory_peak_human"

# Nombre de clés par pattern
Write-Host "`n--- Nombre de clés par type ---"
$zonesCount = (docker exec redis_cache redis-cli -a antennes5g_redis_pass KEYS "zones:*" | Measure-Object -Line).Lines
$clustersCount = (docker exec redis_cache redis-cli -a antennes5g_redis_pass KEYS "clusters:*" | Measure-Object -Line).Lines
Write-Host "Clés zones: $zonesCount"
Write-Host "Clés clusters: $clustersCount"

# Toutes les clés avec TTL
Write-Host "`n--- Clés avec TTL restant ---"
$keys = docker exec redis_cache redis-cli -a antennes5g_redis_pass KEYS "*"
foreach($key in $keys) {
    if($key) {
        $ttl = docker exec redis_cache redis-cli -a antennes5g_redis_pass TTL $key
        Write-Host "$key : $ttl secondes"
    }
}
```

## Script Complet d'Exécution

```powershell
# test_sprint3_redis.ps1
Write-Host "`n╔═══════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  SPRINT 3 - TESTS D'INTÉGRATION CACHE REDIS BACKEND      ║" -ForegroundColor Cyan
Write-Host "╚═══════════════════════════════════════════════════════════╝" -ForegroundColor Cyan

# Test 1: Santé services
Write-Host "`n[1/10] Vérification santé services Docker..." -ForegroundColor White
# ... (copier code Test 1) ...

# Test 2: Cache zones HIT/MISS
Write-Host "`n[2/10] Test cache zones (HIT/MISS)..." -ForegroundColor White
# ... (copier code Test 2) ...

# Test 3: Cache clusters HIT/MISS
Write-Host "`n[3/10] Test cache clusters (HIT/MISS)..." -ForegroundColor White
# ... (copier code Test 3) ...

# Test 4: Filtres
Write-Host "`n[4/10] Test cache avec filtres..." -ForegroundColor White
# ... (copier code Test 4) ...

# Test 5: Invalidation UPDATE
Write-Host "`n[5/10] Test invalidation UPDATE..." -ForegroundColor White
# ... (copier code Test 5) ...

# Test 6: Invalidation DELETE
Write-Host "`n[6/10] Test invalidation DELETE..." -ForegroundColor White
# ... (copier code Test 6) ...

# Test 7: Performance
Write-Host "`n[7/10] Test performance HIT vs MISS..." -ForegroundColor White
# ... (copier code Test 7) ...

# Test 8: Ratio HIT/MISS
Write-Host "`n[8/10] Calcul ratio cache..." -ForegroundColor White
# ... (copier code Test 8) ...

# Test 9: Fallback
Write-Host "`n[9/10] Test fallback sans Redis..." -ForegroundColor White
# ... (copier code Test 9) ...

# Test 10: Monitoring
Write-Host "`n[10/10] Monitoring Redis..." -ForegroundColor White
# ... (copier code Test 10) ...

Write-Host "`n╔═══════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║  TESTS TERMINÉS - SPRINT 3 CACHE REDIS                    ║" -ForegroundColor Green
Write-Host "╚═══════════════════════════════════════════════════════════╝" -ForegroundColor Green
```

## Commandes Rapides Bash (Alternative Linux/Mac)

```bash
#!/bin/bash
# test_sprint3_redis.sh

# Test HIT/MISS zones
curl -i "http://localhost:8082/api/zones/type/commune/simplified?zoom=8" # MISS
curl -i "http://localhost:8082/api/zones/type/commune/simplified?zoom=8" # HIT

# Test HIT/MISS clusters
bbox="minLat=48.8&minLon=2.2&maxLat=48.9&maxLon=2.4"
curl -i "http://localhost:8082/api/antennes/clustered?$bbox&zoom=12" # MISS
curl -i "http://localhost:8082/api/antennes/clustered?$bbox&zoom=12" # HIT

# Vérifier logs
docker logs --tail 50 api_antennes_cpp | grep -E "(Cache HIT|Cache MISS)"

# Redis CLI
docker exec redis_cache redis-cli -a antennes5g_redis_pass KEYS "*"
docker exec redis_cache redis-cli -a antennes5g_redis_pass INFO stats
```

---

**Usage**: Exécuter ces tests après `docker-compose up --build` pour valider l'implémentation Sprint 3
