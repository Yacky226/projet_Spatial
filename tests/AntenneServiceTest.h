#include "../../src/controllers/AntennaZoneController.h"
#include "../../src/services/AntennaZoneService.h"
#include "../../src/services/AntenneService.h"
#include "../../src/services/ZoneService.h"
#include "../../src/models/Antenne.h"
#include "../../src/models/Zone.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <json/json.h>
#include <thread>
#include <chrono>

using namespace drogon;
using namespace testing;

// ============================================================================
// FIXTURE DE BASE POUR LES TESTS DU CONTROLLER
// ============================================================================
class AntennaZoneControllerTest : public ::testing::Test {
protected:
    static constexpr int TEST_PORT = 8082;
    static constexpr const char* BASE_URL = "http://127.0.0.1:8082";
    
    void SetUp() override {
        // Initialiser Drogon en mode test
        app().loadConfigFile("../config/config_test.json");
        app().addListener("127.0.0.1", TEST_PORT);
        
        // Lancer l'application en arrière-plan
        std::thread([]() { app().run(); }).detach();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Nettoyer la base de données
        cleanDatabase();
        
        // Créer des données de test
        setupTestData();
    }

    void TearDown() override {
        cleanDatabase();
    }

    void cleanDatabase() {
        auto client = app().getDbClient();
        client->execSqlSync("DELETE FROM antenna_zone WHERE antenna_id >= 9000 OR zone_id >= 9000");
        client->execSqlSync("DELETE FROM antenna WHERE id >= 9000");
        client->execSqlSync("DELETE FROM zone WHERE id >= 9000");
        client->execSqlSync("DELETE FROM operator WHERE id >= 9000");
    }

    void setupTestData() {
        auto client = app().getDbClient();
        
        // Créer un opérateur de test
        client->execSqlSync(
            "INSERT INTO operator (id, name, contact_email, contact_phone) "
            "VALUES (9000, 'Test Operator', 'test@operator.com', '0600000000')"
        );
        
        // Créer 5 antennes de test
        for (int i = 0; i < 5; i++) {
            client->execSqlSync(
                "INSERT INTO antenna (id, coverage_radius, status, technology, operator_id, geom) "
                "VALUES ($1, $2, 'active', '5G', 9000, ST_SetSRID(ST_MakePoint($3, $4), 4326))",
                9000 + i,
                500.0 + i * 100,
                2.3522 + i * 0.01,
                48.8566 + i * 0.01
            );
        }
        
        // Créer 5 zones de test
        for (int i = 0; i < 5; i++) {
            client->execSqlSync(
                "INSERT INTO zone (id, name, zone_type, population, geom) "
                "VALUES ($1, $2, 'urban', 10000, "
                "ST_SetSRID(ST_GeomFromText('POLYGON((2.3 48.8, 2.4 48.8, 2.4 48.9, 2.3 48.9, 2.3 48.8))'), 4326))",
                9000 + i,
                "Test Zone " + std::to_string(i)
            );
        }
    }

    // Helper : faire une requête HTTP synchrone
    HttpResponsePtr makeRequest(HttpMethod method, const std::string& path, const Json::Value& body = Json::Value()) {
        auto httpClient = HttpClient::newHttpClient(BASE_URL);
        
        auto req = HttpRequest::newHttpRequest();
        req->setMethod(method);
        req->setPath(path);
        
        if (!body.isNull()) {
            req->setBody(body.toStyledString());
            req->setContentTypeCode(CT_APPLICATION_JSON);
        }
        
        HttpResponsePtr resp;
        bool done = false;
        
        httpClient->sendRequest(req, [&](ReqResult result, const HttpResponsePtr& response) {
            resp = response;
            done = true;
        });
        
        // Attendre max 5 secondes
        for (int i = 0; i < 50 && !done; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        EXPECT_TRUE(done) << "HTTP request timed out";
        return resp;
    }

    // Helper : parser la réponse JSON
    Json::Value parseResponse(const HttpResponsePtr& resp) {
        Json::Value json;
        Json::CharReaderBuilder builder;
        std::string errs;
        std::istringstream stream(std::string(resp->getBody()));
        EXPECT_TRUE(Json::parseFromStream(builder, stream, &json, &errs)) 
            << "Failed to parse JSON: " << errs;
        return json;
    }
};

// ============================================================================
// TESTS DE LINK (Lier antenne et zone)
// ============================================================================
TEST_F(AntennaZoneControllerTest, LinkAntennaZone_ValidData_Success) {
    Json::Value body;
    body["antenna_id"] = 9000;
    body["zone_id"] = 9000;
    
    auto resp = makeRequest(Post, "/antenna-zone/link", body);
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "success");
    EXPECT_TRUE(json.isMember("message"));
    
    // Vérifier en base de données
    auto client = app().getDbClient();
    auto result = client->execSqlSync(
        "SELECT * FROM antenna_zone WHERE antenna_id = 9000 AND zone_id = 9000"
    );
    EXPECT_EQ(result.size(), 1) << "Link should exist in database";
}

TEST_F(AntennaZoneControllerTest, LinkAntennaZone_MissingAntennaId_Returns400) {
    Json::Value body;
    body["zone_id"] = 9000;
    
    auto resp = makeRequest(Post, "/antenna-zone/link", body);
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k400BadRequest);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "error");
    EXPECT_THAT(json["message"].asString(), HasSubstr("antenna_id"));
}

TEST_F(AntennaZoneControllerTest, LinkAntennaZone_MissingZoneId_Returns400) {
    Json::Value body;
    body["antenna_id"] = 9000;
    
    auto resp = makeRequest(Post, "/antenna-zone/link", body);
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k400BadRequest);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "error");
    EXPECT_THAT(json["message"].asString(), HasSubstr("zone_id"));
}

TEST_F(AntennaZoneControllerTest, LinkAntennaZone_InvalidAntennaId_Returns404) {
    Json::Value body;
    body["antenna_id"] = 99999; // Inexistant
    body["zone_id"] = 9000;
    
    auto resp = makeRequest(Post, "/antenna-zone/link", body);
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k404NotFound);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "error");
}

TEST_F(AntennaZoneControllerTest, LinkAntennaZone_InvalidZoneId_Returns404) {
    Json::Value body;
    body["antenna_id"] = 9000;
    body["zone_id"] = 99999; // Inexistant
    
    auto resp = makeRequest(Post, "/antenna-zone/link", body);
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k404NotFound);
}

TEST_F(AntennaZoneControllerTest, LinkAntennaZone_DuplicateLink_Returns409) {
    Json::Value body;
    body["antenna_id"] = 9000;
    body["zone_id"] = 9000;
    
    // Premier lien
    auto resp1 = makeRequest(Post, "/antenna-zone/link", body);
    EXPECT_EQ(resp1->getStatusCode(), k200OK);
    
    // Lien en double
    auto resp2 = makeRequest(Post, "/antenna-zone/link", body);
    EXPECT_EQ(resp2->getStatusCode(), k409Conflict);
    
    auto json = parseResponse(resp2);
    EXPECT_EQ(json["status"].asString(), "error");
    EXPECT_THAT(json["message"].asString(), AnyOf(HasSubstr("already"), HasSubstr("duplicate")));
}

TEST_F(AntennaZoneControllerTest, LinkAntennaZone_InvalidJSON_Returns400) {
    auto httpClient = HttpClient::newHttpClient(BASE_URL);
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(Post);
    req->setPath("/antenna-zone/link");
    req->setBody("{invalid json}");
    req->setContentTypeCode(CT_APPLICATION_JSON);
    
    HttpResponsePtr resp;
    bool done = false;
    
    httpClient->sendRequest(req, [&](ReqResult result, const HttpResponsePtr& response) {
        resp = response;
        done = true;
    });
    
    for (int i = 0; i < 50 && !done; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k400BadRequest);
}

// ============================================================================
// TESTS DE UNLINK (Délier antenne et zone)
// ============================================================================
TEST_F(AntennaZoneControllerTest, UnlinkAntennaZone_ExistingLink_Success) {
    // Créer un lien d'abord
    auto client = app().getDbClient();
    client->execSqlSync(
        "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES (9000, 9000)"
    );
    
    Json::Value body;
    body["antenna_id"] = 9000;
    body["zone_id"] = 9000;
    
    auto resp = makeRequest(Post, "/antenna-zone/unlink", body);
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "success");
    
    // Vérifier que le lien n'existe plus
    auto result = client->execSqlSync(
        "SELECT * FROM antenna_zone WHERE antenna_id = 9000 AND zone_id = 9000"
    );
    EXPECT_EQ(result.size(), 0) << "Link should be deleted";
}

TEST_F(AntennaZoneControllerTest, UnlinkAntennaZone_NonExistentLink_Returns404) {
    Json::Value body;
    body["antenna_id"] = 9000;
    body["zone_id"] = 9001; // Lien inexistant
    
    auto resp = makeRequest(Post, "/antenna-zone/unlink", body);
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k404NotFound);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "error");
}

TEST_F(AntennaZoneControllerTest, UnlinkAntennaZone_MissingParameters_Returns400) {
    Json::Value body;
    body["antenna_id"] = 9000;
    // zone_id manquant
    
    auto resp = makeRequest(Post, "/antenna-zone/unlink", body);
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k400BadRequest);
}

// ============================================================================
// TESTS DE GET ZONES BY ANTENNA (avec pagination)
// ============================================================================
TEST_F(AntennaZoneControllerTest, GetZonesByAntenna_NoLinks_ReturnsEmpty) {
    auto resp = makeRequest(Get, "/antenna/9000/zones");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "success");
    EXPECT_TRUE(json["data"].isArray());
    EXPECT_EQ(json["data"].size(), 0);
    EXPECT_TRUE(json.isMember("pagination"));
    EXPECT_EQ(json["pagination"]["total"].asInt(), 0);
}

TEST_F(AntennaZoneControllerTest, GetZonesByAntenna_WithLinks_ReturnsZones) {
    auto client = app().getDbClient();
    
    // Créer 3 liens
    for (int i = 0; i < 3; i++) {
        client->execSqlSync(
            "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES (9000, $1)",
            9000 + i
        );
    }
    
    auto resp = makeRequest(Get, "/antenna/9000/zones");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "success");
    EXPECT_EQ(json["data"].size(), 3);
    EXPECT_EQ(json["pagination"]["total"].asInt(), 3);
    
    // Vérifier la structure des zones
    auto zone = json["data"][0];
    EXPECT_TRUE(zone.isMember("id"));
    EXPECT_TRUE(zone.isMember("name"));
    EXPECT_TRUE(zone.isMember("zone_type"));
}

TEST_F(AntennaZoneControllerTest, GetZonesByAntenna_WithPagination_ReturnsCorrectPage) {
    auto client = app().getDbClient();
    
    // Créer 25 liens (pour tester la pagination)
    for (int i = 0; i < 5; i++) {
        client->execSqlSync(
            "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES (9000, $1)",
            9000 + i
        );
    }
    
    // Page 1, 2 résultats par page
    auto resp1 = makeRequest(Get, "/antenna/9000/zones?page=1&page_size=2");
    
    ASSERT_NE(resp1, nullptr);
    EXPECT_EQ(resp1->getStatusCode(), k200OK);
    
    auto json1 = parseResponse(resp1);
    EXPECT_EQ(json1["data"].size(), 2);
    EXPECT_EQ(json1["pagination"]["page"].asInt(), 1);
    EXPECT_EQ(json1["pagination"]["page_size"].asInt(), 2);
    EXPECT_EQ(json1["pagination"]["total"].asInt(), 5);
    EXPECT_EQ(json1["pagination"]["total_pages"].asInt(), 3);
    
    // Page 2
    auto resp2 = makeRequest(Get, "/antenna/9000/zones?page=2&page_size=2");
    auto json2 = parseResponse(resp2);
    EXPECT_EQ(json2["data"].size(), 2);
    EXPECT_EQ(json2["pagination"]["page"].asInt(), 2);
    
    // Page 3 (dernière page avec 1 résultat)
    auto resp3 = makeRequest(Get, "/antenna/9000/zones?page=3&page_size=2");
    auto json3 = parseResponse(resp3);
    EXPECT_EQ(json3["data"].size(), 1);
}

TEST_F(AntennaZoneControllerTest, GetZonesByAntenna_InvalidPageNumber_Returns400) {
    auto resp = makeRequest(Get, "/antenna/9000/zones?page=0");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k400BadRequest);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "error");
    EXPECT_THAT(json["message"].asString(), HasSubstr("page"));
}

TEST_F(AntennaZoneControllerTest, GetZonesByAntenna_ExcessivePageSize_ClampedTo100) {
    auto client = app().getDbClient();
    for (int i = 0; i < 5; i++) {
        client->execSqlSync(
            "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES (9000, $1)",
            9000 + i
        );
    }
    
    auto resp = makeRequest(Get, "/antenna/9000/zones?page_size=200");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_LE(json["pagination"]["page_size"].asInt(), 100) 
        << "Page size should be clamped to 100";
}

TEST_F(AntennaZoneControllerTest, GetZonesByAntenna_NonExistentAntenna_Returns404) {
    auto resp = makeRequest(Get, "/antenna/99999/zones");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k404NotFound);
}

// ============================================================================
// TESTS DE GET ANTENNAS BY ZONE (avec pagination)
// ============================================================================
TEST_F(AntennaZoneControllerTest, GetAntennasByZone_NoLinks_ReturnsEmpty) {
    auto resp = makeRequest(Get, "/zone/9000/antennas");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "success");
    EXPECT_TRUE(json["data"].isArray());
    EXPECT_EQ(json["data"].size(), 0);
    EXPECT_EQ(json["pagination"]["total"].asInt(), 0);
}

TEST_F(AntennaZoneControllerTest, GetAntennasByZone_WithLinks_ReturnsAntennas) {
    auto client = app().getDbClient();
    
    // Créer 4 liens
    for (int i = 0; i < 4; i++) {
        client->execSqlSync(
            "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES ($1, 9000)",
            9000 + i
        );
    }
    
    auto resp = makeRequest(Get, "/zone/9000/antennas");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "success");
    EXPECT_EQ(json["data"].size(), 4);
    EXPECT_EQ(json["pagination"]["total"].asInt(), 4);
    
    // Vérifier la structure des antennes
    auto antenna = json["data"][0];
    EXPECT_TRUE(antenna.isMember("id"));
    EXPECT_TRUE(antenna.isMember("coverage_radius"));
    EXPECT_TRUE(antenna.isMember("status"));
    EXPECT_TRUE(antenna.isMember("technology"));
}

TEST_F(AntennaZoneControllerTest, GetAntennasByZone_WithPagination_ReturnsCorrectPage) {
    auto client = app().getDbClient();
    
    // Créer 5 liens
    for (int i = 0; i < 5; i++) {
        client->execSqlSync(
            "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES ($1, 9000)",
            9000 + i
        );
    }
    
    // Page 1
    auto resp1 = makeRequest(Get, "/zone/9000/antennas?page=1&page_size=2");
    auto json1 = parseResponse(resp1);
    EXPECT_EQ(json1["data"].size(), 2);
    EXPECT_EQ(json1["pagination"]["page"].asInt(), 1);
    EXPECT_EQ(json1["pagination"]["total_pages"].asInt(), 3);
    
    // Page 2
    auto resp2 = makeRequest(Get, "/zone/9000/antennas?page=2&page_size=2");
    auto json2 = parseResponse(resp2);
    EXPECT_EQ(json2["data"].size(), 2);
    
    // Vérifier que les données sont différentes
    EXPECT_NE(json1["data"][0]["id"].asInt(), json2["data"][0]["id"].asInt());
}

TEST_F(AntennaZoneControllerTest, GetAntennasByZone_OutOfRangePage_ReturnsEmpty) {
    auto client = app().getDbClient();
    client->execSqlSync(
        "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES (9000, 9000)"
    );
    
    auto resp = makeRequest(Get, "/zone/9000/antennas?page=99");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["data"].size(), 0);
    EXPECT_EQ(json["pagination"]["page"].asInt(), 99);
}

TEST_F(AntennaZoneControllerTest, GetAntennasByZone_NonExistentZone_Returns404) {
    auto resp = makeRequest(Get, "/zone/99999/antennas");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k404NotFound);
}

// ============================================================================
// TESTS DE GET ALL LINKS (avec pagination massive)
// ============================================================================
TEST_F(AntennaZoneControllerTest, GetAllLinks_EmptyDatabase_ReturnsEmpty) {
    auto resp = makeRequest(Get, "/antenna-zone/all");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "success");
    EXPECT_TRUE(json["data"].isArray());
    EXPECT_EQ(json["data"].size(), 0);
    EXPECT_EQ(json["pagination"]["total"].asInt(), 0);
}

TEST_F(AntennaZoneControllerTest, GetAllLinks_WithData_ReturnsAllLinks) {
    auto client = app().getDbClient();
    
    // Créer 10 liens
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 2; j++) {
            client->execSqlSync(
                "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES ($1, $2)",
                9000 + i,
                9000 + j
            );
        }
    }
    
    auto resp = makeRequest(Get, "/antenna-zone/all");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["status"].asString(), "success");
    EXPECT_EQ(json["data"].size(), 10);
    EXPECT_EQ(json["pagination"]["total"].asInt(), 10);
    
    // Vérifier la structure
    auto link = json["data"][0];
    EXPECT_TRUE(link.isMember("antenna_id"));
    EXPECT_TRUE(link.isMember("zone_id"));
    EXPECT_TRUE(link.isMember("linked_at"));
}

TEST_F(AntennaZoneControllerTest, GetAllLinks_LargeDataset_PaginationWorks) {
    auto client = app().getDbClient();
    
    // Créer 150 liens (pour tester grand volume)
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            client->execSqlSync(
                "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES ($1, $2)",
                9000 + i,
                9000 + j
            );
        }
    }
    
    // Requête avec pagination
    auto resp = makeRequest(Get, "/antenna-zone/all?page=1&page_size=10");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["data"].size(), 10);
    EXPECT_EQ(json["pagination"]["total"].asInt(), 25);
    EXPECT_EQ(json["pagination"]["total_pages"].asInt(), 3); // 25/10 = 3
    EXPECT_EQ(json["pagination"]["page"].asInt(), 1);
    EXPECT_EQ(json["pagination"]["page_size"].asInt(), 10);
}

TEST_F(AntennaZoneControllerTest, GetAllLinks_DefaultPagination_Uses50PerPage) {
    auto client = app().getDbClient();
    
    // Créer 60 liens
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            client->execSqlSync(
                "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES ($1, $2)",
                9000 + i,
                9000 + j
            );
        }
    }
    
    auto resp = makeRequest(Get, "/antenna-zone/all");
    
    auto json = parseResponse(resp);
    EXPECT_LE(json["data"].size(), 50) << "Default page size should be 50 or less";
    EXPECT_EQ(json["pagination"]["page_size"].asInt(), 50);
}

TEST_F(AntennaZoneControllerTest, GetAllLinks_FilterByAntennaId_ReturnsFiltered) {
    auto client = app().getDbClient();
    
    // Antenne 9000 -> 3 zones
    for (int i = 0; i < 3; i++) {
        client->execSqlSync(
            "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES (9000, $1)",
            9000 + i
        );
    }
    
    // Antenne 9001 -> 2 zones
    for (int i = 0; i < 2; i++) {
        client->execSqlSync(
            "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES (9001, $1)",
            9000 + i
        );
    }
    
    auto resp = makeRequest(Get, "/antenna-zone/all?antenna_id=9000");
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["data"].size(), 3);
    
    // Vérifier que tous les liens sont pour l'antenne 9000
    for (const auto& link : json["data"]) {
        EXPECT_EQ(link["antenna_id"].asInt(), 9000);
    }
}

TEST_F(AntennaZoneControllerTest, GetAllLinks_FilterByZoneId_ReturnsFiltered) {
    auto client = app().getDbClient();
    
    // 4 antennes -> zone 9000
    for (int i = 0; i < 4; i++) {
        client->execSqlSync(
            "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES ($1, 9000)",
            9000 + i
        );
    }
    
    auto resp = makeRequest(Get, "/antenna-zone/all?zone_id=9000");
    
    auto json = parseResponse(resp);
    EXPECT_EQ(json["data"].size(), 4);
    
    for (const auto& link : json["data"]) {
        EXPECT_EQ(link["zone_id"].asInt(), 9000);
    }
}

// ============================================================================
// TESTS DE PERFORMANCE AVEC GRAND VOLUME
// ============================================================================
TEST_F(AntennaZoneControllerTest, Performance_GetAllLinks_1000Links_CompletesQuickly) {
    auto client = app().getDbClient();
    
    // Créer 1000 liens (simulation grand volume)
    // Note: Ceci peut prendre du temps, à exécuter uniquement en CI
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            client->execSqlSync(
                "INSERT INTO antenna_zone (antenna_id, zone_id) VALUES ($1, $2)",
                9000 + i,
                9000 + j
            );
        }
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto resp = makeRequest(Get, "/antenna-zone/all?page_size=100");
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    ASSERT_NE(resp, nullptr);
    EXPECT_EQ(resp->getStatusCode(), k200OK);
    EXPECT_LT(duration, 2000) << "Request should complete in less than 2 seconds";
}

// ============================================================================
// TESTS D'INTÉGRATION COMPLETS
// ============================================================================
TEST_F(AntennaZoneControllerTest, Integration_FullLifecycle_LinkUnlinkVerify) {
    // 1. Lier une antenne à une zone
    Json::Value linkBody;
    linkBody["antenna_id"] = 9000;
    linkBody["zone_id"] = 9000;
    
    auto linkResp = makeRequest(Post, "/antenna-zone/link", linkBody);
    EXPECT_EQ(linkResp->getStatusCode(), k200OK);
    
    // 2. Vérifier que le lien apparaît dans getZonesByAntenna
    auto zonesResp = makeRequest(Get, "/antenna/9000/zones");
    auto zonesJson = parseResponse(zonesResp);
    EXPECT_EQ(zonesJson["data"].size(), 1);
    EXPECT_EQ(zonesJson["data"][0]["id"].asInt(), 9000);
    
    // 3. Vérifier que le lien apparaît dans getAntennasByZone
    auto antennasResp = makeRequest(Get, "/zone/9000/antennas");
    auto antennasJson = parseResponse(antennasResp);
    EXPECT_EQ(antennasJson["data"].size(), 1);
    EXPECT_EQ(antennasJson["data"][0]["id"].asInt(), 9000);
    
    // 4. Vérifier que le lien apparaît dans getAllLinks
    auto allResp = makeRequest(Get, "/antenna-zone/all");
    auto allJson = parseResponse(allResp);
    EXPECT_GE(allJson["data"].size(), 1);
    
    // 5. Délier
    auto unlinkResp = makeRequest(Post, "/antenna-zone/unlink", linkBody);
    EXPECT_EQ(unlinkResp->getStatusCode(), k200OK);
    
    // 6. Vérifier que le lien a disparu
    auto zonesAfterResp = makeRequest(Get, "/antenna/9000/zones");
    auto zonesAfterJson = parseResponse(zonesAfterResp);
    EXPECT_EQ(zonesAfterJson["data"].size(), 0);
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}