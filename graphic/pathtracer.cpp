#include "pathtracer.h"

#include <iostream>
#include <cassert>

#include "graphic.h"


namespace unisim
{

std::vector<std::string> g_PathTracerCommonSrcs;


// INTERFACE //

PathTracerInterface::PathTracerInterface(const std::shared_ptr<GraphicProgram>& program) :
    GpuProgramInterface(program)
{
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

bool PathTracerProvider::definePathTracerInterface(GraphicContext& context, PathTracerInterface& interface)
{
    return true;
}

void PathTracerProvider::setPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const
{
}

}
