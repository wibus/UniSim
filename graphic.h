#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <string>
#include <vector>

#include "resource.h"


namespace unisim
{

class Scene;
class Camera;

std::string loadSource(const std::string& fileName);

bool generateVertexShader(
        GLuint& shaderId,
        const std::string& fileName,
        const std::vector<std::string>& defines = {});

bool generateFragmentShader(
        GLuint& shaderId,
        const std::string& fileName,
        const std::vector<std::string>& defines = {});

bool generateComputerShader(
        GLuint& shaderId,
        const std::string& fileName,
        const std::vector<std::string>& defines = {});

bool generateGraphicProgram(
        GLuint& programId,
        const std::string& vertexFileName,
        const std::string& fragmentFileName,
        const std::vector<std::string>& defines = {});

bool generateComputeProgram(
        GLuint& programId,
        const std::string& computeFileName,
        const std::vector<std::string>& defines = {});

bool generateComputeProgram(
        GLuint& programId,
        const std::string programName,
        const std::vector<GLuint>& shaders);


struct GraphicSettings
{
    bool unbiased;
};

struct GraphicContext
{
    const Scene& scene;
    const Camera& camera;
    ResourceManager& resources;
    const GraphicSettings& settings;
};


class GraphicTask : public PathTracerProvider
{
public:
    GraphicTask(const std::string& name);
    virtual ~GraphicTask();

    const std::string& name() const { return _name; }

    virtual void registerDynamicResources(GraphicContext& context) {}
    virtual bool defineResources(GraphicContext& context) { return true; }

    virtual void update(GraphicContext& context) {}
    virtual void render(GraphicContext& context) {}

protected:
    bool addPathTracerModule(GLuint shaderId);

    bool addPathTracerModule(
            GLuint& shaderId,
            const GraphicSettings& settings,
            const std::string& computeFileName,
            const std::vector<std::string>& defines = {});

private:
    std::string _name;
};


class GraphicTaskGraph
{
public:
    GraphicTaskGraph();

    bool initialize(const Scene& scene, const Camera& camera);

    void execute(const Scene& scene, const Camera& camera);

    const ResourceManager& resources() const { return _resources; }

private:
    void createTaskGraph(const Scene& scene, const Camera& camera);
    void addTask(const std::shared_ptr<GraphicTask>& task);

    GraphicSettings _settings;
    ResourceManager _resources;
    std::vector<std::shared_ptr<GraphicTask>> _tasks;
};


}

#endif // GRAPHIC_H
