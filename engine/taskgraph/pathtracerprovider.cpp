#include "pathtracerprovider.h"

#include <algorithm>

#include <PilsCore/Utils/Logger.h>

#include "../resource/primitive.h"


namespace unisim
{

std::vector<std::string> g_PathTracerCommonSrcs;


// PATH TRACER INTERFACE //

PathTracerInterface::PathTracerInterface() :
    GpuProgramInterface()
{
}


// PATH TRACER MODULE //
PathTracerModule::PathTracerModule(const std::string& name, const std::shared_ptr<GraphicShader>& shader) :
    _name(name),
    _shader(shader)
{
    PILS_INFO("Creating path tracer module '", _name, "' from shader '", shader->name(), "'");
}

PathTracerModule::~PathTracerModule()
{
    PILS_INFO("Destroying path tracer module '", _name, "'");
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
    PathTracerInterface& interface)
{
    return true;
}

void PathTracerProviderTask::bindPathTracerResources(
    GraphicContext& context,
    CompiledGpuProgramInterface& compiledGpi) const
{
}

bool PathTracerProviderTask::addPathTracerModule(
    std::vector<std::shared_ptr<PathTracerModule>>& modules,
    const std::string& name,
    const GraphicSettings& settings,
    const std::string& computeFileName,
    const std::vector<std::string>& defines)
{
    std::vector<std::string> allDefines = defines;

    if(settings.unbiased)
        allDefines.push_back("IS_UNBIASED");

    for(int t = 0; t < Primitive::Type_Count; ++t)
    {
        std::string upperName = Primitive::Type_Names[t];
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
        allDefines.push_back("PRIMITIVE_TYPE_" + upperName + " " + std::to_string(t));
    }

    std::vector<std::string> allSrcs = g_PathTracerCommonSrcs;
    allSrcs.push_back(loadSource(computeFileName));

    std::shared_ptr<GraphicShader> shader = 0;
    if(!generateShader(shader, ShaderType::Compute, computeFileName, allSrcs, allDefines))
        return false;

    modules.push_back(std::make_shared<PathTracerModule>(name, shader));

    return true;
}

}
