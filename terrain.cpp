#include "terrain.h"

#include "scene.h"
#include "object.h"
#include "mesh.h"
#include "material.h"
#include "profiler.h"


namespace unisim
{

DeclareProfilePoint(Terrain);
DeclareProfilePointGpu(Terrain);

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

bool NoTerrain::Task::defineResources(GraphicContext& context)
{
    return true;
}

bool NoTerrain::Task::definePathTracerModules(GraphicContext& context)
{
    GLuint moduleId = 0;
    if(!addPathTracerModule(moduleId, context.settings, "shaders/noterrain.glsl"))
        return false;

    return true;
}


FlatTerrain::FlatTerrain() :
    _height(0)
{
    _task = std::make_shared<Task>();

    std::shared_ptr<Mesh> mesh(new Mesh(PrimitiveType::Terrain, 0.0));

    std::shared_ptr<Material> material(new Material("Terrain"));
    material->setDefaultAlbedo(glm::vec3(0.2, 0.2, 0.2));
    material->setDefaultRoughness(0.3);
    material->setDefaultMetalness(0.0f);

    _object.reset(new Object("Terrain", nullptr, mesh, material, nullptr));

    addObject(_object);
}

void FlatTerrain::setMaterial(const std::shared_ptr<Material>& material)
{
    clearObjects();
    _object->setMaterial(material);
    addObject(_object);
}

std::shared_ptr<GraphicTask> FlatTerrain::graphicTask()
{
    return _task;
}

FlatTerrain::Task::Task() :
    GraphicTask("Flat Terrain")
{

}

bool FlatTerrain::Task::defineResources(GraphicContext& context)
{
    return true;
}

bool FlatTerrain::Task::definePathTracerModules(GraphicContext& context)
{
    GLuint moduleId = 0;
    if(!addPathTracerModule(moduleId, context.settings, "shaders/flatterrain.glsl"))
        return false;

    return true;
}

void FlatTerrain::Task::setPathTracerResources(GraphicContext& context, GLuint programId, GLuint& textureUnitStart) const
{
    FlatTerrain& terrain = *dynamic_cast<FlatTerrain*>(context.scene.terrain().get());

    glUniform1f(glGetUniformLocation(programId, "terrainHeight"), terrain.height());
}

}
