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


Terrain::Terrain() :
    _objectOffset(-1)
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

std::vector<GLuint> NoTerrain::pathTracerShaders() const
{
    return {static_cast<Task&>(*_task).shader()};
}

GLuint NoTerrain::setPathTracerResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const
{
    return textureUnitStart;
}

NoTerrain::Task::Task() :
    GraphicTask("No Terrain"),
    _shaderId(0)
{

}

bool NoTerrain::Task::defineResources(GraphicContext& context)
{
    if(!generatePathTracerModule(_shaderId, context.settings, "shaders/noterrain.glsl"))
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

std::vector<GLuint> FlatTerrain::pathTracerShaders() const
{
    return {static_cast<Task&>(*_task).shader()};
}

GLuint FlatTerrain::setPathTracerResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const
{
    glUniform1f(glGetUniformLocation(programId, "terrainHeight"), _height);
    glUniform1f(glGetUniformLocation(programId, "terrainObject0"), objectOffset());

    return textureUnitStart;
}

FlatTerrain::Task::Task() :
    GraphicTask("Flat Terrain"),
    _shaderId(0)
{

}

bool FlatTerrain::Task::defineResources(GraphicContext& context)
{
    if(!generatePathTracerModule(_shaderId, context.settings, "shaders/flatterrain.glsl"))
        return false;

    return true;
}

}
