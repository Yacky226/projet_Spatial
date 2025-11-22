@echo off
setlocal enabledelayedexpansion

title Antennes 5G API - Quick Start
cls

echo ================================
echo   ANTENNES 5G API - QUICK START
echo ================================
echo.

echo [1/4] Vérification de Docker...
docker --version >nul 2>&1 || (
    echo ERREUR : Docker n'est pas installe.
    pause
    exit /b 1
)
echo OK
echo.

echo [2/4] Nettoyage du volume Postgres obsolete...
docker volume ls --format "{{.Name}}" | findstr /i "postgres_data" >nul
if %errorlevel%==0 (
    docker-compose down -v >nul 2>&1
    echo Volume supprime.
) else (
    echo Aucun volume existant.
)
echo.

echo [3/4] Build + démarrage des services...
docker-compose up -d --build
if errorlevel 1 (
    echo ERREUR lors du build ou du demarrage.
    pause
    exit /b 1
)
echo OK
echo.

echo [4/4] Attente du service Postgres...
:waitloop
docker inspect -f "{{.State.Health.Status}}" postgis_antennes5g 2>nul | findstr /i "healthy" >nul
if %errorlevel%==0 (
    echo PostGIS est operationnel.
) else (
    echo En attente...
    timeout /t 3 >nul
    goto waitloop
)
echo.

echo ================================
echo       STATUS DES SERVICES
echo ================================
docker-compose ps
echo.

echo ================================
echo       ENDPOINTS DISPONIBLES
echo ================================
echo GET  http://localhost:8080/health
echo GET  http://localhost:8080/api/test/db
echo GET  http://localhost:8080/api/test/tables
echo GET  http://localhost:8080/api/test/stats
echo.
echo GET  http://localhost:8080/api/antennes
echo GET  http://localhost:8080/api/antennes/geojson
echo GET  http://localhost:8080/api/antennes/{id}
echo.

echo ================================
echo       COMMANDES UTILES
echo ================================
echo Logs temps reel :  docker-compose logs -f api
echo Arreter services : docker-compose down
echo Rebuild complet :  docker-compose up -d --build
echo.

pause
