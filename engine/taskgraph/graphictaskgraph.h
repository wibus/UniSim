#ifndef GRAPHICTASKGRAPH_H
#define GRAPHICTASKGRAPH_H

#include "../graphic/gpudevice.h"

#include "graphictask.h"
#include "pathtracerprovider.h"


namespace unisim
{

class ImGuiRenderer;


class GraphicTaskGraph
{
public:
    GraphicTaskGraph();

    bool initialize(const View& view, const Scene& scene, const Camera& camera);

    bool reloadShaders(const View& view, const Scene& scene, const Camera& camera);

    void execute(const View& view, const Scene& scene, const Camera& camera);

    GpuDevice& device() { return _device; }
    const GpuDevice& device() const { return _device; }

    const GpuResourceManager& resources() const { return _resources; }

    const ImGuiRenderer& imGuiRenderer() const { return *_imGuiRenderer; }

private:
    void createTaskGraph(const Scene& scene);
    void addTask(const GraphicTaskPtr& task);

    GpuDevice _device;
    GraphicSettings _settings;
    GpuResourceManager _resources;
    std::vector<GraphicTaskPtr> _tasks;
    std::unique_ptr<ImGuiRenderer> _imGuiRenderer;

    std::shared_ptr<PathTracerTask> _pathTracerTask;
};

}

#endif // GRAPHICTASKGRAPH_H
