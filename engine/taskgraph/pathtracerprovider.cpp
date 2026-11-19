#include "pathtracerprovider.h"

#include <PilsCore/Utils/Logger.h>


namespace unisim
{


// PATH TRACER MODULE //
PathTracerModule::PathTracerModule(
        const std::string& name,
        const std::shared_ptr<ShaderSource>& source,
        const std::vector<std::string>& defines) :
    _name(name),
    _source(source),
    _defines(defines)
{
    PILS_DEBUG("Creating path tracer module '", _name, "' from shader '", source ? source->uri() : "null", "'");
}

PathTracerModule::~PathTracerModule()
{
    PILS_DEBUG("Destroying path tracer module '", _name, "'");
}


// PATH TRACER TASK //
PathTracerProviderTask::PathTracerProviderTask(const std::string& name) :
    GraphicTask(name),
    _name(name),
    _hash(0)
{
}

PathTracerProviderTask::~PathTracerProviderTask()
{
}

bool PathTracerProviderTask::definePathTracerModules(
    GraphicContext& context,
    std::vector<std::shared_ptr<PathTracerModule>>& modules)
{
    return true;
}

bool PathTracerProviderTask::definePathTracerInterface(
    GraphicContext& context,
    ProgramInterfaceTemplate& piTemplate)
{
    return true;
}

void PathTracerProviderTask::bindPathTracerResources(
    GraphicContext& context,
    ProgramInterface& pi) const
{
}

bool PathTracerProviderTask::addPathTracerModule(
    std::vector<std::shared_ptr<PathTracerModule>>& modules,
    const std::string& name,
    const std::string& computeFileName,
    const std::vector<std::string>& defines)
{
    std::shared_ptr<ShaderSource> source(new ShaderSource(computeFileName));

    if (!source->load(computeFileName))
        return false;

    modules.push_back(std::make_shared<PathTracerModule>(name, source, defines));

    return true;
}

}
