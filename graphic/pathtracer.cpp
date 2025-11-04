#include "pathtracer.h"

#include <iostream>
#include <cassert>

#include "graphic.h"


namespace unisim
{

std::vector<std::string> g_PathTracerCommonSrcs;


// INTERFACE //

PathTracerInterface::PathTracerInterface(const std::shared_ptr<GraphicProgram>& program) :
    _program(program),
    _nextTextureUnit(0),
    _nextUboBindPoint(0),
    _nextSsboBindPoint(0)
{
    if (declareUbo("PathTracerCommonParams") &&
        declareSsbo("Primitives") &&
        declareSsbo("Meshes") &&
        declareSsbo("Spheres") &&
        declareSsbo("Planes") &&
        declareSsbo("Instances") &&
        declareSsbo("BvhNodes") &&
        declareSsbo("Triangles") &&
        declareSsbo("VerticesPos") &&
        declareSsbo("VerticesData") &&
        declareSsbo("Emitters") &&
        declareSsbo("DirectionalLights") &&
        declareSsbo("Textures") &&
        declareSsbo("Materials"))
    {
        std::cout << "Path tracer interface initialized" << std::endl;
    }
    else
    {
        _program.reset();
        std::cerr << "Could not declare path tracer input binding points" << std::endl;
        assert(false);
    }
}

bool PathTracerInterface::declareUbo(const std::string& blockName)
{
    assert(_uboBindPoints.find(blockName) == _uboBindPoints.end());

    GLuint location = glGetProgramResourceIndex(_program->handle(), GL_UNIFORM_BLOCK, blockName.c_str());

    if(location == GL_INVALID_ENUM)
    {
        std::cerr << "Invalid uniform interface type\n";
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        std::cerr << "Could not find uniform block named " << blockName << std::endl;
        return false;
    }

    glShaderStorageBlockBinding(_program->handle(), location, _nextUboBindPoint);
    _uboBindPoints[blockName] = _nextUboBindPoint;
    ++_nextUboBindPoint;

    return true;
}

bool PathTracerInterface::declareSsbo(const std::string& blockName)
{
    assert(_ssboBindPoints.find(blockName) == _ssboBindPoints.end());

    GLuint location = glGetProgramResourceIndex(_program->handle(), GL_SHADER_STORAGE_BLOCK, blockName.c_str());

    if(location == GL_INVALID_ENUM)
    {
        std::cerr << "Invalid uniform interface type\n";
        return false;
    }

    if(location == GL_INVALID_INDEX)
    {
        std::cerr << "Could not find SSBO block named " << blockName << std::endl;
        return false;
    }

    glShaderStorageBlockBinding(_program->handle(), location, _nextSsboBindPoint);
    _ssboBindPoints[blockName] = _nextSsboBindPoint;
    ++_nextSsboBindPoint;

    return true;
}


GLuint PathTracerInterface::getUboBindPoint(const std::string& blockName) const
{
    auto it = _uboBindPoints.find(blockName);
    assert(it != _ssboBindPoints.end());
    if(it != _uboBindPoints.end())
        return it->second;

    return GL_INVALID_INDEX;
}

GLuint PathTracerInterface::getSsboBindPoint(const std::string& blockName) const
{
    auto it = _ssboBindPoints.find(blockName);
    assert(it != _ssboBindPoints.end());
    if(it != _ssboBindPoints.end())
        return it->second;

    return GL_INVALID_INDEX;
}

GLuint PathTracerInterface::grabTextureUnit()
{
    return _nextTextureUnit++;
}

void PathTracerInterface::resetTextureUnits()
{
    _nextTextureUnit = 0;
}

bool PathTracerInterface::isValid() const
{
    return _program && _program->isValid();
}


// MODULE //

PathTracerModule::PathTracerModule(const std::string& name) :
    _name(name)
{
    std::cout << "Creating path tracer module '" << _name << "'" << std::endl;
}

PathTracerModule::PathTracerModule(const std::string& name, const std::shared_ptr<GraphicShader>& shader) :
    _name(name),
    _shader(shader)
{
    std::cout << "Creating path tracer module '" << _name << "' from shader '" << shader->name() << "'" << std::endl;
}

PathTracerModule::~PathTracerModule()
{
    std::cout << "Destroying path tracer module '" << _name << "'" << std::endl;
}

void PathTracerModule::reset(const std::shared_ptr<GraphicShader>& shader)
{
    std::cout << "Resetting path tracer module '" << _name << "'" << std::endl;

    _shader = shader;
}


// PROVIDER //
PathTracerProvider::PathTracerProvider() :
    _hash(0)
{
}

PathTracerProvider::~PathTracerProvider()
{
}

std::vector<std::shared_ptr<PathTracerModule>> PathTracerProvider::pathTracerModules() const
{
    return {};
}

bool PathTracerProvider::definePathTracerModules(GraphicContext& context)
{
    return true;
}

void PathTracerProvider::setPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const
{
}

}
