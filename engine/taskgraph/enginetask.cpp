#include "enginetask.h"

#include <iostream>

#include "../physics/gravitytask.h"


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
    const GpuResourceManager& resources)
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
    const GpuResourceManager& resources)
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
    
    addTask(EngineTaskPtr(new GravityTask()));
}

void EngineTaskGraph::addTask(const EngineTaskPtr& task)
{
    _tasks.push_back(task);
}

}
