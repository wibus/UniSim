#include "pathtracerscene.h"

#include "../object.h"
#include "../body.h"
#include "../mesh.h"
#include "../material.h"
#include "../sky.h"


namespace unisim
{

std::shared_ptr<Object> makeObject(const std::string& name, double radius, const glm::dvec3& position, Object* parent = nullptr)
{
    std::shared_ptr<Body> body(new Body(radius, 1, true));
    body->setPosition(position);

    std::shared_ptr<Mesh> mesh(new Mesh(true, radius));

    std::shared_ptr<Material> material(new Material(name));

    std::shared_ptr<Object> object(new Object(name, body, mesh, material, parent));

    return object;
}

std::shared_ptr<DirectionalLight> makeDirLight(const std::string& name, const glm::dvec3& position, const glm::dvec3& emission, double luminance, double solidAngle)
{
    std::shared_ptr<DirectionalLight> light(new DirectionalLight(name));

    light->setDirection(glm::normalize(position));
    light->setEmissionColor(emission);
    light->setEmissionLuminance(luminance);
    light->setSolidAngle(solidAngle);

    return light;
}

PathTracerScene::PathTracerScene() :
    Scene("Path Tracer")
{
    _sky.reset(new SkySphere("textures/syferfontein_0d_clear_puresky_4k.exr"));
    _sky->setExposure(glm::pow(2.0f, 10.0f));

    auto light = makeObject("Light", 1, {0, 7.5, 9});
    auto floor = makeObject("Floor", 1000, {0, 0, -1000});
    auto ceiling = makeObject("Ceiling", 1000, {0, 0, 1015});
    auto wallLeft = makeObject("Wall Left", 1000, {-1015, 0, 0});
    auto wallRight = makeObject("Wall Right", 1000, {1015, 0, 0});
    auto wallFront = makeObject("Wall Front", 1000, {0, -1030, 0});
    auto ballLeft = makeObject("Ball Left", 2, {-3, 12, 2});
    auto ballRight = makeObject("Ball Right", 2, {3, 12, 2});
    auto ballFront = makeObject("Ball Front", 1, {0, 10, 1});

    light->material()->setDefaultEmissionColor({1, 1, 1});
    light->material()->setDefaultEmissionLuminance(25);

    light->material()->setDefaultAlbedo({0.5, 0.5, 0.5});
    floor->material()->setDefaultAlbedo({0.8, 0.8, 0.8});
    ceiling->material()->setDefaultAlbedo({0.9, 0.9, 0.9});
    wallLeft->material()->setDefaultAlbedo({0.8, 0.2, 0.2});
    wallRight->material()->setDefaultAlbedo({0.2, 0.8, 0.2});
    wallFront->material()->setDefaultAlbedo({0.8, 0.8, 0.8});
    ballLeft->material()->setDefaultAlbedo({0.8, 0.8, 0.8});
    ballRight->material()->setDefaultAlbedo({0.9, 0.9, 0.9});
    ballFront->material()->setDefaultAlbedo({1.0, 0.85, 0.03});

    ballLeft->material()->loadAlbedo("textures/mars_albedo.jpg");

    ballLeft->material()->setDefaultRoughness(2);

    ballRight->material()->setDefaultMetalness(0);
    ballRight->material()->setDefaultRoughness(0.0);
    ballRight->material()->setDefaultReflectance(0.04);

    ballFront->material()->setDefaultRoughness(0.2);
    ballFront->material()->setDefaultMetalness(1);

    _objects.push_back(light);
    _objects.push_back(floor);
    _objects.push_back(ceiling);
    _objects.push_back(wallLeft);
    _objects.push_back(wallRight);
    _objects.push_back(wallFront);
    _objects.push_back(ballLeft);
    _objects.push_back(ballRight);
    _objects.push_back(ballFront);

    auto sun = makeDirLight("Sun", glm::dvec3(2, -1, 1), glm::dvec3(1, 0.9, 0.7), 1.6e9, 60.0e-6);
    _directionalLights.push_back(sun);
}

}
