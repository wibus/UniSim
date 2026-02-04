#ifndef GRAPHICTASKGRAPH_H
#define GRAPHICTASKGRAPH_H

#include "../graphic/gpudevice.h"

#include "graphictask.h"
#include "pathtracerprovider.h"


namespace unisim
{

class GraphicTaskGraph
{
public:
    GraphicTaskGraph();

    bool initialize(const View& view, const Scene& scene, const Camera& camera);

    bool reloadShaders(const View& view, const Scene& scene, const Camera& camera);

    void execute(const View& view, const Scene& scene, const Camera& camera);

    const GpuResourceManager& resources() const { return _resources; }

private:
    void createTaskGraph(const Scene& scene);
    void addTask(const GraphicTaskPtr& task);

    GpuDevice _device;
    GraphicSettings _settings;
    GpuResourceManager _resources;
    std::vector<GraphicTaskPtr> _tasks;

    std::shared_ptr<PathTracerTask> _pathTracerTask;
};

}

#endif // GRAPHICTASKGRAPH_H
