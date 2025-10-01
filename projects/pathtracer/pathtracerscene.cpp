#include "pathtracerscene.h"

#include "../engine/instance.h"
#include "../engine/body.h"
#include "../engine/primitive.h"
#include "../engine/camera.h"
#include "../engine/material.h"
#include "../engine/sky.h"
#include "../engine/terrain.h"


namespace unisim
{

std::shared_ptr<Instance> makeSphere(const std::string& name, double radius, const glm::dvec3& position, Instance* parent = nullptr)
{
    std::shared_ptr<Body> body(new Body(radius, 1, true));
    body->setPosition(position);

    std::shared_ptr<Material> material(new Material(name));
    std::shared_ptr<Primitive> primitive(new Sphere(radius));
    primitive->setMaterial(material);

    std::shared_ptr<Instance> instance(new Instance(name, body, {primitive}, parent));

    return instance;
}

std::shared_ptr<Instance> makeCube(const std::string& name, double length, float uvScale, const glm::dvec3& position, Instance* parent = nullptr)
{
    std::shared_ptr<Body> body(new Body(length, 1, true));
    body->setPosition(position);

    std::shared_ptr<Material> material(new Material(name));
    std::shared_ptr<Primitive> primitive(new Mesh(Mesh::cube(length, uvScale)));
    primitive->setMaterial(material);

    std::shared_ptr<Instance> instance(new Instance(name, body, {primitive}, parent));

    return instance;
}

void PathTracerScene::initializeCamera(Camera& camera)
{
    camera.setEV(-0.4);
}

PathTracerScene::PathTracerScene() :
    Scene("Path Tracer")
{
    _sky.reset(new PhysicalSky());

    std::shared_ptr<FlatTerrain> terrain(new FlatTerrain(6));
    std::shared_ptr<Material> terrainMaterial(new Material("Grass Terrain"));
    terrainMaterial->loadAlbedo("textures/grass/Grass_albedo.jpg");
    terrainMaterial->loadSpecular("textures/grass/Grass_specular.jpg");
    terrain->setMaterial(terrainMaterial);
    _terrain = std::static_pointer_cast<Terrain>(terrain);

    SkyLocalization& localization = _sky->localization();
    localization.setLongitude(-73.567);
    localization.setLatitude(45.509);
    localization.setTimeOfDay(5.12f);
    localization.setDayOfYear(81.0f);
    localization.setDayOfMoon(17.0);

    auto light = makeSphere("Light", 1, {0, 7.5, 9});
    light->primitives()[0]->material()->setDefaultAlbedo({0.5, 0.5, 0.5});
    light->primitives()[0]->material()->setDefaultEmissionColor({1, 1, 1});
    light->primitives()[0]->material()->setDefaultEmissionLuminance(25);
    _instances.push_back(light);

    auto ballLeft = makeSphere("Ball Left", 2, {-3, 12, 2});
    ballLeft->primitives()[0]->material()->setDefaultAlbedo({0.8, 0.8, 0.8});
    ballLeft->primitives()[0]->material()->loadAlbedo("textures/mars_albedo.jpg");
    ballLeft->primitives()[0]->material()->setDefaultRoughness(2);
    _instances.push_back(ballLeft);

    auto ballRight = makeSphere("Ball Right", 2, {3, 12, 2});
    ballRight->primitives()[0]->material()->setDefaultAlbedo({0.9, 0.9, 0.9});
    ballRight->primitives()[0]->material()->setDefaultMetalness(0);
    ballRight->primitives()[0]->material()->setDefaultRoughness(0.0);
    ballRight->primitives()[0]->material()->setDefaultReflectance(0.04);
    ballRight->body()->setQuaternion(quat(glm::dvec3(0, 0, 1), glm::pi<double>()));
    _instances.push_back(ballRight);

    auto ballFront = makeSphere("Ball Front", 1, {0, 10, 1});
    ballFront->primitives()[0]->material()->setDefaultAlbedo({1.0, 0.85, 0.03});
    ballFront->primitives()[0]->material()->setDefaultRoughness(0.2);
    ballFront->primitives()[0]->material()->setDefaultMetalness(1);
    _instances.push_back(ballFront);

    auto cubeLeft = makeCube("Cube Left", 100, 1, glm::dvec3(-80, 11, 50));
    cubeLeft->body()->setQuaternion(quat(glm::dvec3(0, 0, 1), glm::pi<double>() * 0.0));
    cubeLeft->primitives()[0]->material()->setDefaultAlbedo({0.9, 0.9, 0.9});
    cubeLeft->primitives()[0]->material()->setDefaultRoughness(0.5);
    cubeLeft->primitives()[0]->material()->setDefaultMetalness(0);
    _instances.push_back(cubeLeft);

    auto cubeRight = makeCube("Cube Right", 100, 16, glm::dvec3(60, 11, -30));
    cubeRight->body()->setQuaternion(quat(glm::dvec3(0, 0, 1), glm::pi<double>() * 0.0));
    cubeRight->primitives()[0]->material()->setDefaultAlbedo({0.9, 0.9, 0.9});
    cubeRight->primitives()[0]->material()->setDefaultRoughness(0.5);
    cubeRight->primitives()[0]->material()->setDefaultMetalness(0);
    cubeRight->primitives()[0]->material()->loadAlbedo("textures/granite/Granite_albedo.jpg");
    cubeRight->primitives()[0]->material()->loadSpecular("textures/granite/Granite_specular.jpg");
    _instances.push_back(cubeRight);
}

}
