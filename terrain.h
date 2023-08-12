#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>

#include <GLM/glm.hpp>

#include "graphic.h"


namespace unisim
{

class Instance;
class Material;
class Plane;


class Terrain
{
public:
    Terrain();
    virtual ~Terrain();

    const std::vector<std::shared_ptr<Instance>>& instances() const { return _instances; }

    virtual std::shared_ptr<GraphicTask> graphicTask() = 0;

protected:
    void clearInstances() { _instances.clear(); }
    void addInstance(const std::shared_ptr<Instance>& instance) { _instances.push_back(instance); }

private:
    std::vector<std::shared_ptr<Instance>> _instances;
};


class NoTerrain : public Terrain
{
public:
    NoTerrain();

    std::shared_ptr<GraphicTask> graphicTask() override;

private:
    class Task : public GraphicTask
    {
    public:
        Task();

        bool definePathTracerModules(GraphicContext& context) override;

        bool defineResources(GraphicContext& context) override;
    };

    std::shared_ptr<Task> _task;
};


class FlatTerrain : public Terrain
{
public:
    FlatTerrain(float uvScale = 1.0f);

    double height() const { return _height; }
    void setHeight(double height) { _height = height; }

    void setMaterial(const std::shared_ptr<Material>& material);

    std::shared_ptr<GraphicTask> graphicTask() override;

private:
    class Task : public GraphicTask
    {
    public:
        Task();

        bool definePathTracerModules(GraphicContext& context) override;

        bool defineResources(GraphicContext& context) override;
};

    double _height;
    std::shared_ptr<Plane> _plane;
    std::shared_ptr<Task> _task;
};

}

#endif // TERRAIN_H
