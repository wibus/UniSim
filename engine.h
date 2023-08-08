#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <memory>
#include <vector>


namespace unisim
{

class Scene;
class Camera;
class ResourceManager;

class UiEngineTask;
class Gravity;


struct EngineSettings
{

};


struct EngineContext
{
    const double dt;
    Scene& scene;
    Camera& camera;
    const ResourceManager& resources;
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


class EngineTaskGraph
{
public:
    EngineTaskGraph();

    bool initialize(Scene& scene,
            Camera& camera,
            const std::shared_ptr<UiEngineTask>& ui,
            const ResourceManager& resources);

    void execute(
            double dt,
            Scene& scene,
            Camera& camera,
            const ResourceManager& resources);

private:
    void createTaskGraph(EngineContext& context);
    void addTask(const std::shared_ptr<EngineTask>& task);

    EngineSettings _settings;
    std::vector<std::shared_ptr<EngineTask>> _tasks;

    std::shared_ptr<UiEngineTask> _ui;
};

}

#endif // ENGINE_H
