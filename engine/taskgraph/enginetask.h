#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <memory>
#include <vector>


namespace unisim
{

class Scene;
class Camera;
class GpuResourceManager;


struct EngineSettings
{

};


struct EngineContext
{
    const double dt;
    Scene& scene;
    Camera& camera;
    const GpuResourceManager& resources;
    const EngineSettings& settings;
};


class EngineTask
{
public:
    EngineTask(const std::string& name);
    virtual ~EngineTask();

    const std::string& name() const { return _name; }

    virtual bool initialize(EngineContext& context);

    virtual void update(EngineContext& context);

private:
    std::string _name;
};

typedef std::shared_ptr<EngineTask> EngineTaskPtr;


class EngineTaskGraph
{
public:
    EngineTaskGraph();

    bool initialize(Scene& scene,
            Camera& camera,
                    const GpuResourceManager& resources);

    void execute(
            double dt,
            Scene& scene,
            Camera& camera,
        const GpuResourceManager& resources);

private:
    void createTaskGraph(const Scene& scene);
    void addTask(const EngineTaskPtr& task);

    EngineSettings _settings;
    std::vector<EngineTaskPtr> _tasks;
};

}

#endif // ENGINE_H
