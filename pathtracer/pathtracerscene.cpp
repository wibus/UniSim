#include "pathtracerscene.h"

#include "../object.h"
#include "../body.h"
#include "../primitive.h"
#include "../camera.h"
#include "../material.h"
#include "../sky.h"
#include "../terrain.h"


namespace unisim
{

std::shared_ptr<Object> makeObject(const std::string& name, double radius, const glm::dvec3& position, Object* parent = nullptr)
{
    std::shared_ptr<Body> body(new Body(radius, 1, true));
    body->setPosition(position);

    std::shared_ptr<Material> material(new Material(name));
    std::shared_ptr<Primitive> primitive(new Sphere(radius));
    primitive->setMaterial(material);

    std::shared_ptr<Object> object(new Object(name, body, {primitive}, parent));

    return object;
}

void PathTracerScene::initializeCamera(Camera& camera)
{
    camera.setEV(-0.4);
}

PathTracerScene::PathTracerScene() :
    Scene("Path Tracer")
{
    _sky.reset(new PhysicalSky());
    _terrain.reset(new FlatTerrain());

    SkyLocalization& localization = _sky->localization();
    localization.setLongitude(-73.567);
    localization.setLatitude(45.509);
    localization.setTimeOfDay(5.12f);
    localization.setDayOfYear(81.0f);
    localization.setDayOfMoon(17.0);

    auto light = makeObject("Light", 1, {0, 7.5, 9});
    light->primitives()[0]->material()->setDefaultAlbedo({0.5, 0.5, 0.5});
    light->primitives()[0]->material()->setDefaultEmissionColor({1, 1, 1});
    light->primitives()[0]->material()->setDefaultEmissionLuminance(25);
    _objects.push_back(light);

    auto ballLeft = makeObject("Ball Left", 2, {-3, 12, 2});
    ballLeft->primitives()[0]->material()->setDefaultAlbedo({0.8, 0.8, 0.8});
    ballLeft->primitives()[0]->material()->loadAlbedo("textures/mars_albedo.jpg");
    ballLeft->primitives()[0]->material()->setDefaultRoughness(2);
    _objects.push_back(ballLeft);

    auto ballRight = makeObject("Ball Right", 2, {3, 12, 2});
    ballRight->primitives()[0]->material()->setDefaultAlbedo({0.9, 0.9, 0.9});
    ballRight->primitives()[0]->material()->setDefaultMetalness(0);
    ballRight->primitives()[0]->material()->setDefaultRoughness(0.0);
    ballRight->primitives()[0]->material()->setDefaultReflectance(0.04);
    ballRight->body()->setQuaternion(quat(glm::dvec3(0, 0, 1), glm::pi<double>()));
    _objects.push_back(ballRight);

    auto ballFront = makeObject("Ball Front", 1, {0, 10, 1});
    ballFront->primitives()[0]->material()->setDefaultAlbedo({1.0, 0.85, 0.03});
    ballFront->primitives()[0]->material()->setDefaultRoughness(0.2);
    ballFront->primitives()[0]->material()->setDefaultMetalness(1);
    _objects.push_back(ballFront);
}

}
