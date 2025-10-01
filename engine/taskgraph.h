#ifndef TASKGRAPH_H
#define TASKGRAPH_H

#include "graphic.h"
#include "context.h"
#include "resource.h"
#include "pathtracer.h"


namespace unisim
{

class GraphicTask : public PathTracerProvider
{
public:
    GraphicTask(const std::string& name);
    virtual ~GraphicTask();

    const std::string& name() const { return _name; }

    std::shared_ptr<GraphicProgram> registerProgram(const std::string& name);
    std::shared_ptr<PathTracerModule> registerPathTracerModule(const std::string& name);

    std::vector<std::shared_ptr<PathTracerModule>> pathTracerModules() const override;

    virtual void registerDynamicResources(Context& context) {}
    virtual bool defineShaders(Context& context) { return true; }
    virtual bool defineResources(Context& context) { return true; }

    virtual void update(Context& context) {}
    virtual void render(Context& context) {}

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

    void render(Context& context) override;
};


class GraphicTaskGraph
{
public:
    GraphicTaskGraph();

    bool initialize(const Scene& scene, const Camera& camera);

    bool reloadShaders(const Scene& scene, const Camera& camera);

    void execute(const Scene& scene, const Camera& camera);

    const ResourceManager& resources() const { return _resources; }

private:
    void createTaskGraph(const Scene& scene);
    void addTask(const std::shared_ptr<GraphicTask>& task);

    GraphicSettings _settings;
    ResourceManager _resources;
    std::vector<std::shared_ptr<GraphicTask>> _tasks;
};

}

#endif // TASKGRAPH_H
