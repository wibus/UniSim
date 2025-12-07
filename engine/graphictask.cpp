#include "graphictask.h"

#include <algorithm>
#include <iostream>

#include "../system/profiler.h"

#include "../resource/primitive.h"

#include "../graphic/view.h"

#include "light.h"
#include "scene.h"
#include "sky.h"
#include "bvh.h"
#include "grading.h"
#include "tracer.h"
#include "ui.h"
#include "camera.h"
#include "materialdatabase.h"
#include "terrain.h"

namespace unisim
{

GraphicTask::GraphicTask(const std::string& name) :
    _name(name)
{
}

GraphicTask::~GraphicTask()
{
}

std::shared_ptr<GraphicProgram> GraphicTask::registerProgram(const std::string& name)
{
    _programs.emplace_back(new GraphicProgram(name));
    return _programs.back();
}

std::shared_ptr<PathTracerModule> GraphicTask::registerPathTracerModule(const std::string& name)
{
    _modules.emplace_back(new PathTracerModule(name));
    return _modules.back();
}

std::vector<std::shared_ptr<PathTracerModule>> GraphicTask::pathTracerModules() const
{
    return _modules;
}

bool GraphicTask::addPathTracerModule(
    PathTracerModule& module,
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

    module.reset(shader);

    return true;
}


ClearSwapChain::ClearSwapChain() :
    GraphicTask("Clear")
{

}

DefineProfilePointGpu(Clear);

void ClearSwapChain::render(GraphicContext& context)
{
    ProfileGpu(Clear);


    context.view.setViewport();
    const Viewport& viewport = context.camera.viewport();
    glViewport(0, 0, viewport.width, viewport.height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


GraphicTaskGraph::GraphicTaskGraph()
{
    _settings.unbiased = false;
}

bool GraphicTaskGraph::initialize(const View& view, const Scene& scene, const Camera& camera)
{
    createTaskGraph(scene);

    GraphicContext context = {_device, view, scene, camera, _resources, _settings};

    for(const auto& task : _tasks)
    {
        task->registerDynamicResources(context);
    }

    _resources.initialize();

    g_GlslExtensions += "#extension GL_ARB_bindless_texture : require\n";

    g_PathTracerCommonSrcs.clear();
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/constants.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/data.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/inputs.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/signatures.glsl"));

    for(const auto& task : _tasks)
    {
        bool ok = task->definePathTracerModules(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' path tracer module definition failed\n";
            return false;
        }

        _resources.registerPathTracerProvider(std::dynamic_pointer_cast<PathTracerProvider>(task));
    }

    for(const auto& task : _tasks)
    {
        bool ok = task->defineShaders(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' shader definition failed\n";
            return false;
        }
    }

    for(const auto& task : _tasks)
    {
        bool ok = task->defineResources(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' resource definition failed\n";
            return false;
        }
    }

    return true;
}

bool GraphicTaskGraph::reloadShaders(const View& view, const Scene& scene, const Camera& camera)
{
    g_PathTracerCommonSrcs.clear();
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/constants.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/data.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/inputs.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/signatures.glsl"));

    GraphicContext context = {_device, view, scene, camera, _resources, _settings};

    for(const auto& task : _tasks)
    {
        bool ok = task->definePathTracerModules(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' path tracer module definition failed\n";
            return false;
        }

        _resources.registerPathTracerProvider(std::dynamic_pointer_cast<PathTracerProvider>(task));
    }

    for(const auto& task : _tasks)
    {
        bool ok = task->defineShaders(context);

        if(!ok)
        {
            std::cerr << "'" << task->name() << "' shader definition failed\n";
            return false;
        }
    }

    return true;
}

void GraphicTaskGraph::execute(const View& view, const Scene& scene, const Camera& camera)
{
    GraphicContext context = {_device, view, scene, camera, _resources, _settings};

    for(const auto& task : _tasks)
    {
        task->update(context);
    }

    for(const auto& task : _tasks)
    {
        task->render(context);
    }
}

void GraphicTaskGraph::createTaskGraph(const Scene& scene)
{
    _tasks.clear();

    addTask(scene.materials());
    addTask(std::shared_ptr<GraphicTask>(new BVH()));
    addTask(scene.sky()->graphicTask());
    addTask(scene.terrain()->graphicTask());
    addTask(std::shared_ptr<GraphicTask>(new Lighting()));
    addTask(std::shared_ptr<GraphicTask>(new PathTracer()));
    addTask(std::shared_ptr<GraphicTask>(new ClearSwapChain()));
    addTask(std::shared_ptr<GraphicTask>(new ColorGrading()));
    addTask(std::shared_ptr<GraphicTask>(new Ui()));
}

void GraphicTaskGraph::addTask(const std::shared_ptr<GraphicTask>& task)
{
    _tasks.push_back(task);
}

}
