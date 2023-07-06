#include "graphic.h"

#include "radiation.h"
#include "scene.h"
#include "sky.h"


namespace unisim
{

GraphicTask::GraphicTask(const std::string& name) :
    _name(name)
{

}

GraphicTask::~GraphicTask()
{

}


GraphicTaskGraph::GraphicTaskGraph()
{

}

bool GraphicTaskGraph::initialize(const Scene& scene, const Camera& camera)
{
    createTaskGraph(scene, camera);

    GraphicContext context = {scene, camera, _resources};

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
            return false;
        }
    }

    return true;
}

void GraphicTaskGraph::execute(const Scene& scene, const Camera& camera)
{
    GraphicContext context = {scene, camera, _resources};

    for(const auto& task : _tasks)
    {
        task->update(context);
    }

    for(const auto& task : _tasks)
    {
        task->render(context);
    }
}

void GraphicTaskGraph::createTaskGraph(const Scene& scene, const Camera& camera)
{
    addTask(scene.sky()->createGraphicTask());
    addTask(std::make_shared<Radiation>());
}

void GraphicTaskGraph::addTask(const std::shared_ptr<GraphicTask>& task)
{
    _tasks.push_back(task);
}

}
