#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>

#include <GLM/glm.hpp>

#include "graphic.h"


namespace unisim
{

class Object;
class Material;


class Terrain
{
public:
    Terrain();
    virtual ~Terrain();

    const std::vector<std::shared_ptr<Object>>& objects() const { return _objects; }

    virtual std::shared_ptr<GraphicTask> graphicTask() = 0;

protected:
    void clearObjects() { _objects.clear(); }
    void addObject(const std::shared_ptr<Object>& object) { _objects.push_back(object); }

private:
    std::vector<std::shared_ptr<Object>> _objects;
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
    FlatTerrain();

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

        void setPathTracerResources(GraphicContext& context, GLuint programId, GLuint& textureUnitStart) const override;
    };

    double _height;
    std::shared_ptr<Object> _object;
    std::shared_ptr<Task> _task;
};

}

#endif // TERRAIN_H
