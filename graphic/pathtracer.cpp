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


// PROVIDER //
PathTracerProvider::PathTracerProvider() :
    _hash(0)
{
}

PathTracerProvider::~PathTracerProvider()
{
}

bool PathTracerProvider::definePathTracerModules(GraphicContext& context, std::vector<std::shared_ptr<PathTracerModule>>& modules)
{
    return true;
}

bool PathTracerProvider::definePathTracerInterface(GraphicContext& context, PathTracerInterface& interface)
{
    return true;
}

void PathTracerProvider::bindPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const
{
}

}
