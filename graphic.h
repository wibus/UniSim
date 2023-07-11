#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <string>
#include <vector>

#include "resource.h"


namespace unisim
{

class Scene;
class Camera;

bool generateVertexShader(
        GLuint& shaderId,
        const std::string& fileName);

bool generateFragmentShader(
        GLuint& shaderId,
        const std::string& fileName);

bool generateComputerShader(
        GLuint& shaderId,
        const std::string& fileName);

bool generateGraphicProgram(
        GLuint& programId,
        const std::string& vertexFileName,
        const std::string& fragmentFileName);

bool generateComputeProgram(
        GLuint& programId,
        const std::string& computeFileName,
        const std::vector<GLuint>& shaders = {});


struct GraphicContext
{
    const Scene& scene;
    const Camera& camera;
    ResourceManager& resources;
};


class GraphicTask
{
public:
    GraphicTask(const std::string& name);
    virtual ~GraphicTask();

    const std::string& name() const { return _name; }

    virtual void registerDynamicResources(GraphicContext& context) {}
    virtual bool defineResources(GraphicContext& context) { return true; }

    virtual void update(GraphicContext& context) {}
    virtual void render(GraphicContext& context) {}

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

    ResourceManager _resources;
    std::vector<std::shared_ptr<GraphicTask>> _tasks;
};


}

#endif // GRAPHIC_H
