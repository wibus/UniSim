#ifndef GRAPHICTASK_H
#define GRAPHICTASK_H

#include "../graphic/graphic.h"
#include "../graphic/gpudevice.h"
#include "../graphic/gpuresource.h"
#include "../graphic/pathtracer.h"


namespace unisim
{

class View;
class Scene;
class Camera;
class PathTracer;

struct GraphicSettings
{
    bool unbiased;
};

struct GraphicContext
{
    GpuDevice& device;

    const View& view;
    const Scene& scene;
    const Camera& camera;

    GpuResourceManager& resources;
    const GraphicSettings& settings;
};

class GraphicTask : public PathTracerProvider
{
public:
    GraphicTask(const std::string& name);
    virtual ~GraphicTask();

    const std::string& name() const { return _name; }

    std::shared_ptr<GraphicProgram> registerProgram(const std::string& name);
    std::shared_ptr<PathTracerModule> registerPathTracerModule(const std::string& name);

    std::vector<std::shared_ptr<PathTracerModule>> pathTracerModules() const override;

    virtual void registerDynamicResources(GraphicContext& context) {}
    virtual bool defineShaders(GraphicContext& context) { return true; }
    virtual bool defineResources(GraphicContext& context) { return true; }

    virtual void update(GraphicContext& context) {}
    virtual void render(GraphicContext& context) {}

protected:
    bool addPathTracerModule(
        PathTracerModule& module,
        const GraphicSettings& settings,
        const std::string& computeFileName,
        const std::vector<std::string>& defines = {});

private:
    std::string _name;

    std::vector<std::shared_ptr<GraphicProgram>> _programs;
    std::vector<std::shared_ptr<PathTracerModule>> _modules;
};


class ClearSwapChain : public GraphicTask
{
public:
    ClearSwapChain();

    void render(GraphicContext& context) override;
};


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
    void addTask(const std::shared_ptr<GraphicTask>& task);

    GpuDevice _device;
    GraphicSettings _settings;
    GpuResourceManager _resources;
    std::vector<std::shared_ptr<GraphicTask>> _tasks;

    std::shared_ptr<PathTracer> _pathTracerTask;
};

}

#endif // GRAPHICTASK_H
