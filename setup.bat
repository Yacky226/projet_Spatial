@echo off
REM Script de configuration et déploiement pour Windows
REM Antennes 5G API avec Docker

setlocal enabledelayedexpansion

echo.
echo ============================================
echo   ANTENNES 5G API - Setup Windows
echo ============================================
echo.

REM Vérifier si Docker est installé
docker --version >nul 2>&1
if errorlevel 1 (
    echo [ERREUR] Docker n'est pas installé ou pas dans le PATH
    echo Veuillez installer Docker Desktop for Windows
    pause
    exit /b 1
)

echo [OK] Docker détecté

REM Vérifier si Docker Compose est installé
docker-compose --version >nul 2>&1
if errorlevel 1 (
    echo [ERREUR] Docker Compose n'est pas installé
    pause
    exit /b 1
)

echo [OK] Docker Compose détecté
echo.

REM Menu principal
echo Choisissez une action :
echo 1. Démarrer l'application complète
echo 2. Reconstruire l'image Docker
echo 3. Afficher les logs
echo 4. Arrêter l'application
echo 5. Réinitialiser (nettoyer les volumes)
echo 6. Tester la connexion API
echo.

set /p choice="Votre choix (1-6) : "

if "%choice%"=="1" (
    call :start_app
) else if "%choice%"=="2" (
    call :rebuild
) else if "%choice%"=="3" (
    call :show_logs
) else if "%choice%"=="4" (
    call :stop_app
) else if "%choice%"=="5" (
    call :reset
) else if "%choice%"=="6" (
    call :test_api
) else (
    echo [ERREUR] Choix invalide
    pause
    exit /b 1
)

goto :end

:start_app
echo.
echo [INFO] Démarrage de l'application...
docker-compose up -d
if errorlevel 1 (
    echo [ERREUR] Erreur lors du démarrage
    pause
    exit /b 1
)
echo [OK] Application démarrée
echo.
echo Attendez 10-15 secondes pour que l'API démarre...
timeout /t 15
echo.
echo [INFO] État des services :
docker-compose ps
echo.
echo [INFO] Logs de l'API :
docker-compose logs api | tail -20
echo.
goto :end

:rebuild
echo.
echo [INFO] Reconstruction de l'image Docker...
docker-compose build --no-cache
if errorlevel 1 (
    echo [ERREUR] Erreur lors de la compilation
    pause
    exit /b 1
)
echo [OK] Image reconstruite
echo.
goto :end

:show_logs
echo.
echo [INFO] Affichage des logs (Ctrl+C pour quitter)...
echo.
docker-compose logs -f api
goto :end

:stop_app
echo.
echo [INFO] Arrêt de l'application...
docker-compose down
echo [OK] Application arrêtée
goto :end

:reset
echo.
echo [AVERTISSEMENT] Cette action supprimera tous les volumes et données !
set /p confirm="Êtes-vous sûr ? (O/N) : "
if /i "%confirm%"=="O" (
    echo [INFO] Suppression des volumes...
    docker-compose down -v
    echo [OK] Volumes supprimés
) else (
    echo [INFO] Annulé
)
goto :end

:test_api
echo.
echo [INFO] Test de l'API...
echo.
timeout /t 2
echo Test 1 : Health Check
curl -s http://localhost:8080/health | findstr /v "^$"
echo.
echo.
echo Test 2 : Database Connection
curl -s http://localhost:8080/api/test/db | findstr /v "^$"
echo.
echo.
echo Test 3 : Tables Info
curl -s http://localhost:8080/api/test/tables | findstr /v "^$"
echo.
echo.
echo Test 4 : Get All Antennes
curl -s http://localhost:8080/api/antennes | findstr /v "^$"
echo.
goto :end

:end
echo.
echo Terminé.
pause