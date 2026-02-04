#include "graphictaskgraph.h"

#include <iostream>
#include <algorithm>

#include "../system/profiler.h"

#include "../resource/primitive.h"

#include "../graphic/view.h"

#include "../camera.h"
#include "../scene.h"
#include "../ui.h"

#include "../sky/skytask.h"
#include "../bvh/geometrytask.h"
#include "../bvh/lighttask.h"
#include "../bvh/materialtask.h"
#include "../grading/gradingtask.h"
#include "../pathtracer/pathtracertask.h"
#include "../terrain/terraintask.h"

namespace unisim
{

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

    for(const auto& task : _tasks)
    {
        bool ok = task->defineResources(context);

        if(!ok)
        {
            PILS_ERROR("'", task->name(), "' resource definition failed\n");
            return false;
        }
    }

    g_GlslExtensions += "#extension GL_ARB_bindless_texture : require\n";

    g_PathTracerCommonSrcs.clear();
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/constants.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/data.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/inputs.glsl"));
    g_PathTracerCommonSrcs.push_back(loadSource("shaders/common/signatures.glsl"));

    for(const auto& task : _tasks)
    {
        bool ok = task->defineShaders(context);

        if(!ok)
        {
            PILS_ERROR("'", task->name(), "' shader definition failed\n");
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
    // Task declaration
    _pathTracerTask.reset(new PathTracerTask());

    // Task Graph
    _tasks.clear();

    addTask(GraphicTaskPtr(new TerrainTask()));
    addTask(GraphicTaskPtr(new MaterialTask()));
    addTask(GraphicTaskPtr(new GeometryTask()));
    addTask(GraphicTaskPtr(new SkyTask()));
    addTask(GraphicTaskPtr(new LightTask()));
    addTask(_pathTracerTask);
    addTask(GraphicTaskPtr(new ClearSwapChain()));
    addTask(GraphicTaskPtr(new GradingTask()));
    addTask(GraphicTaskPtr(new Ui()));

    // Path Tracer Providers
    std::vector<PathTracerProviderTaskPtr> pathTracerProviders;
    for(const auto& task : _tasks)
    {
        if (auto pathTracerProvider = std::dynamic_pointer_cast<PathTracerProviderTask>(task))
        {
            pathTracerProviders.push_back(pathTracerProvider);
        }
    }
    _pathTracerTask->setPathTracerTasks(pathTracerProviders);
}

void GraphicTaskGraph::addTask(const GraphicTaskPtr& task)
{
    _tasks.push_back(task);
}

}
