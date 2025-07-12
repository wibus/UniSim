#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <string>
#include <vector>
#include "resource.h"


namespace unisim
{

class Scene;
class Camera;
class MaterialDatabase;


struct GraphicSettings
{
    bool unbiased;
};

struct GraphicContext
{
    const Scene& scene;
    const Camera& camera;
    ResourceManager& resources;
    const MaterialDatabase& materials;
    const GraphicSettings& settings;
};

class GraphicProgram
{
    GraphicProgram(const GraphicProgram&) = delete;

    friend class GraphicTask;
    GraphicProgram(const std::string& name);
public:
    ~GraphicProgram();

    bool isValid() const { return _programId != 0; }

    std::string name() const { return _name; }

    GLuint programId() const { return _programId; }

    void reset(GLuint programId, const std::vector<GLuint>& shaders);

private:
    std::string _name;
    GLuint _programId;
    std::vector<GLuint> _shaders;
};

using GraphicProgramPtr = std::shared_ptr<GraphicProgram>;

class PathTracerModule
{
    PathTracerModule(const PathTracerModule&) = delete;

    friend class GraphicTask;
    PathTracerModule(const std::string& name);

public:
    ~PathTracerModule();

    GLuint shaderId() const { return _shaderId; }

    std::string name() const { return _name; }

    void reset(GLuint shaderId);

private:
    std::string _name;
    GLuint _shaderId;
};

using PathTracerModulePtr = std::shared_ptr<PathTracerModule>;

class GraphicTask : public PathTracerProvider
{
public:
    GraphicTask(const std::string& name);
    virtual ~GraphicTask();

    const std::string& name() const { return _name; }

    std::shared_ptr<GraphicProgram> registerProgram(const std::string& name);
    std::shared_ptr<PathTracerModule> registerPathTracerModule(const std::string& name);

    std::vector<GLuint> pathTracerModules() const override;

    virtual void registerDynamicResources(GraphicContext& context) {}
    virtual bool defineShaders(GraphicContext& context) { return true; }
    virtual bool defineResources(GraphicContext& context) { return true; }

    virtual void update(GraphicContext& context) {}
    virtual void render(GraphicContext& context) {}

protected:
    bool generateGraphicProgram(
            GraphicProgram& program,
            const std::string& vertexFileName,
            const std::string& fragmentFileName,
            const std::vector<std::string>& defines = {});

    bool generateComputeProgram(
            GraphicProgram& program,
            const std::string& computeFileName,
            const std::vector<std::string>& defines = {});

    bool generateComputeProgram(
            GraphicProgram& program,
            const std::string programName,
            const std::vector<GLuint>& shaders);

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
    std::shared_ptr<MaterialDatabase> _materials;
};


}

#endif // GRAPHIC_H
