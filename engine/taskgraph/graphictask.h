#ifndef GRAPHICTASK_H
#define GRAPHICTASK_H

#include "../graphic/graphic.h"
#include "../graphic/gpuresource.h"
#include "../graphic/gpuprograminterface.h"


namespace unisim
{

class View;
class Scene;
class Camera;
class GpuDevice;
class PathTracerTask;

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

class GraphicTask
{
public:
    GraphicTask(const std::string& name);
    virtual ~GraphicTask();

    const std::string& name() const { return _name; }

    virtual void registerDynamicResources(GraphicContext& context) {}
    virtual bool defineShaders(GraphicContext& context) { return true; }
    virtual bool defineResources(GraphicContext& context) { return true; }

    virtual void update(GraphicContext& context) {}
    virtual void render(GraphicContext& context) {}

private:
    std::string _name;
};

typedef std::shared_ptr<GraphicTask> GraphicTaskPtr;


class ClearSwapChain : public GraphicTask
{
public:
    ClearSwapChain();

    void render(GraphicContext& context) override;
};

}

#endif // GRAPHICTASK_H
