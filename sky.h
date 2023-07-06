#ifndef SKY_H
#define SKY_H

#include <memory>

#include <GLM/glm.hpp>

#include <GL/glew.h>

#include "graphic.h"


namespace unisim
{

struct  Viewport;

class Scene;
class Texture;


class Sky
{
public:
    enum class Mapping {Sphere, Cube};

    Sky(Mapping mapping);
    virtual ~Sky();

    Mapping mapping() const { return _mapping; }

    glm::vec4 quaternion() const { return _quaternion; }
    void setQuaternion(const glm::vec4& quaternion) { _quaternion = quaternion; }

    float exposure() const { return _exposure; }
    void setExposure(float exposure) { _exposure = exposure; }

    virtual unsigned int width() const = 0;
    virtual unsigned int height() const = 0;

    virtual std::shared_ptr<GraphicTask> createGraphicTask() = 0;

private:
    Mapping _mapping;
    glm::vec4 _quaternion;
    float _exposure;
};


class SkySphere : public Sky
{
public:
    SkySphere();
    SkySphere(const std::string& fileName);

    unsigned int width() const override;
    unsigned int height() const override;

    std::shared_ptr<GraphicTask> createGraphicTask() override;

private:
    std::shared_ptr<Texture> _texture;

    class Task : public GraphicTask
    {
    public:
        Task(const std::shared_ptr<Texture>& texture);

        bool defineResources(GraphicContext& context) override;

    private:
        std::shared_ptr<Texture> _texture;
    };

    std::shared_ptr<GraphicTask> _task;
};

}

#endif // SKY_H
