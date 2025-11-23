#include <gtest/gtest.h>
#include "ObstacleController.h"
#include "drogon/drogon.h"

class ObstacleControllerTest : public ::testing::Test {
protected:
    drogon::HttpRequestPtr req;
    std::function<void(const drogon::HttpResponsePtr&)> callback;

    void SetUp() override {
        req = drogon::HttpRequest::newHttpRequest();
        callback = [](const drogon::HttpResponsePtr&) {};
    }
};

TEST_F(ObstacleControllerTest, CreateSuccess) {
    Json::Value json;
    json["type"] = "batiment";
    json["geom_type"] = "POLYGON";
    json["wkt"] = "POLYGON((0 0,1 1,1 0,0 0))";
    req->setJsonObject(json);

    ObstacleController controller;
    controller.create(req, callback);
    // Add assertions for successful creation response
}

TEST_F(ObstacleControllerTest, CreateInvalidJson) {
    req->setJsonObject(nullptr);

    ObstacleController controller;
    controller.create(req, callback);
    // Add assertions for invalid JSON response
}

TEST_F(ObstacleControllerTest, GetAllSuccess) {
    req->setParameter("page", "1");
    req->setParameter("pageSize", "10");

    ObstacleController controller;
    controller.getAll(req, callback);
    // Add assertions for successful retrieval response
}

TEST_F(ObstacleControllerTest, GetByIdSuccess) {
    int id = 1; // Assume this ID exists
    ObstacleController controller;
    controller.getById(req, callback, id);
    // Add assertions for successful retrieval response
}

TEST_F(ObstacleControllerTest, UpdateSuccess) {
    Json::Value json;
    json["type"] = "vegetation";
    req->setJsonObject(json);
    int id = 1; // Assume this ID exists

    ObstacleController controller;
    controller.update(req, callback, id);
    // Add assertions for successful update response
}

TEST_F(ObstacleControllerTest, RemoveSuccess) {
    int id = 1; // Assume this ID exists

    ObstacleController controller;
    controller.remove(req, callback, id);
    // Add assertions for successful removal response
}

TEST_F(ObstacleControllerTest, GetGeoJSONSuccess) {
    req->setParameter("page", "1");
    req->setParameter("pageSize", "10");

    ObstacleController controller;
    controller.getGeoJSON(req, callback);
    // Add assertions for successful GeoJSON retrieval response
}