#include "pathtracerscene.h"

#include "../object.h"
#include "../body.h"
#include "../mesh.h"
#include "../material.h"


namespace unisim
{

std::shared_ptr<Object> makeObject(const std::string& name, double radius, const glm::dvec3& position, Object* parent = nullptr)
{
    std::shared_ptr<Body> body(new Body(radius, 1));
    body->setPosition(position);

    std::shared_ptr<Mesh> mesh(new Mesh(true, radius));

    std::shared_ptr<Material> material(new Material(name));

    std::shared_ptr<Object> object(new Object(name, body, mesh, material, parent));

    return object;
}

PathTracerScene::PathTracerScene()
{
    auto light = makeObject("Light", 1, {0, 5, 8});
    auto floor = makeObject("Floor", 1000, {0, 0, -1002});
    auto ceiling = makeObject("Ceiling", 1000, {0, 0, 1011});
    auto wallLeft = makeObject("Wall Left", 1000, {-1011, 0, 0});
    auto wallRight = makeObject("Wall Right", 1000, {1011, 0, 0});
    auto wallFront = makeObject("Wall Front", 1000, {0, 1021, 0});
    auto ballLeft = makeObject("Ball Left", 0.5, {-3, 12, 0});
    auto ballRight = makeObject("Ball Right", 0.5, {3, 12, 0});

    light->material()->setDefaultEmission({20, 20, 20});

    light->material()->setDefaultAlbedo({0.5, 0.5, 0.5});
    floor->material()->setDefaultAlbedo({0.5, 0.5, 0.5});
    ceiling->material()->setDefaultAlbedo({0.5, 0.5, 0.5});
    wallLeft->material()->setDefaultAlbedo({0.8, 0.2, 0.2});
    wallRight->material()->setDefaultAlbedo({0.2, 0.8, 0.2});
    wallFront->material()->setDefaultAlbedo({0.5, 0.5, 0.5});
    ballLeft->material()->setDefaultAlbedo({0.8, 0.8, 0.8});
    ballRight->material()->setDefaultAlbedo({1.0, 1.0, 1.0});

    _objects.push_back(light);
    _objects.push_back(floor);
    _objects.push_back(ceiling);
    _objects.push_back(wallLeft);
    _objects.push_back(wallRight);
    _objects.push_back(wallFront);
    _objects.push_back(ballLeft);
    _objects.push_back(ballRight);
}

}
