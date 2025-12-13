#include "terrain.h"

#include "../system/units.h"

#include "../resource/body.h"
#include "../resource/instance.h"
#include "../resource/material.h"
#include "../resource/primitive.h"

#include "scene.h"


namespace unisim
{

DefineResource(FlatTerrain);


Terrain::Terrain()
{

}

Terrain::~Terrain()
{

}

void Terrain::ui()
{

}


NoTerrain::NoTerrain()
{
    _task = std::make_shared<Task>();
}

std::shared_ptr<GraphicTask> NoTerrain::graphicTask()
{
    return _task;
}

NoTerrain::Task::Task() :
    GraphicTask("No Terrain")
{

}


FlatTerrain::FlatTerrain(float uvScale) :
    _height(0)
{
    _task = std::make_shared<Task>();

    std::shared_ptr<Material> material(new Material("Terrain"));
    material->setDefaultAlbedo(glm::vec3(0.25, 0.25, 0.25));
    material->setDefaultRoughness(0.7f);
    material->setDefaultMetalness(0.0f);

    _plane.reset(new Plane(uvScale));
    _plane->setMaterial(material);

    std::shared_ptr<Body> body(new Body(1.0f, 1.0f, true));
    body->setPosition(glm::vec3(0, 0, 0));
    body->setQuaternion(quat(glm::vec3(0, 0, 1), 0.0f));

    std::vector<std::shared_ptr<Primitive>> primitives;
    primitives.push_back(std::dynamic_pointer_cast<Primitive>(_plane));
    std::shared_ptr<Instance> instance(new Instance("Terrain", body, primitives, nullptr));

    addInstance(instance);
}

void FlatTerrain::setMaterial(const std::shared_ptr<Material>& material)
{
    _plane->setMaterial(material);
}

std::shared_ptr<GraphicTask> FlatTerrain::graphicTask()
{
    return _task;
}

FlatTerrain::Task::Task() :
    GraphicTask("Flat Terrain")
{

}

}
