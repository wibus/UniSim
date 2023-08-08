#include "terrain.h"

#include "scene.h"
#include "body.h"
#include "object.h"
#include "primitive.h"
#include "material.h"
#include "profiler.h"


namespace unisim
{

DefineResource(FlatTerrain);


Terrain::Terrain()
{

}

Terrain::~Terrain()
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

bool NoTerrain::Task::definePathTracerModules(GraphicContext& context)
{
    return true;
}

bool NoTerrain::Task::defineResources(GraphicContext& context)
{
    return true;
}


FlatTerrain::FlatTerrain() :
    _height(0)
{
    _task = std::make_shared<Task>();

    std::shared_ptr<Material> material(new Material("Terrain"));
    material->setDefaultAlbedo(glm::vec3(0.25, 0.25, 0.25));
    material->setDefaultRoughness(0.7f);
    material->setDefaultMetalness(0.0f);

    _plane.reset(new Plane(1.0f));
    _plane->setMaterial(material);

    std::shared_ptr<Body> body(new Body(1.0f, 1.0f, true));
    body->setPosition(glm::vec3(0, 0, 0));
    body->setQuaternion(quat(glm::vec3(0, 0, 1), 0.0f));

    std::vector<std::shared_ptr<Primitive>> primitives;
    primitives.push_back(std::dynamic_pointer_cast<Primitive>(_plane));
    std::shared_ptr<Object> object(new Object("Terrain", body, primitives, nullptr));

    addObject(object);
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

bool FlatTerrain::Task::definePathTracerModules(GraphicContext& context)
{
    return true;
}

bool FlatTerrain::Task::defineResources(GraphicContext& context)
{
    return true;
}

}
