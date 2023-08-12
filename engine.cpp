#include "engine.h"

#include "ui.h"
#include "gravity.h"


namespace unisim
{

EngineTask::EngineTask(const std::string& name) :
    _name(name)
{

}

EngineTask::~EngineTask()
{

}

bool EngineTask::initialize(EngineContext &context)
{
    return true;
}

void EngineTask::update(EngineContext& context)
{

}


EngineTaskGraph::EngineTaskGraph()
{
    _settings = {};
}

bool EngineTaskGraph::initialize(
        Scene& scene,
        Camera& camera,
        const ResourceManager& resources)
{
    createTaskGraph(scene);

    EngineContext context = {0, scene, camera, resources, _settings};

    for(const auto& task : _tasks)
    {
        bool ok = task->initialize(context);

        if(!ok)
        {
            std::cerr << task->name() << " initialization failed\n";
            return false;
        }
    }

    return true;
}

void EngineTaskGraph::execute(
        double dt,
        Scene& scene,
        Camera& camera,
        const ResourceManager& resources)
{
    EngineContext context = {dt, scene, camera, resources, _settings};

    for(const auto& task : _tasks)
    {
        task->update(context);
    }
}

void EngineTaskGraph::createTaskGraph(const Scene& scene)
{
    _tasks.clear();

    addTask(std::shared_ptr<EngineTask>(new Gravity()));
}

void EngineTaskGraph::addTask(const std::shared_ptr<EngineTask>& task)
{
    _tasks.push_back(task);
}

}
