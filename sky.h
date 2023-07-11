#ifndef SKY_H
#define SKY_H

#include <memory>

#include <GLM/glm.hpp>

#include <GL/glew.h>

#include "graphic.h"


namespace atmosphere
{
namespace reference
{
struct AtmosphereParameters;
}
class Model;
}


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

    virtual std::shared_ptr<GraphicTask> graphicTask() = 0;

    virtual std::vector<GLuint> shaders() const = 0;

    virtual GLuint setProgramResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const = 0;

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

    std::shared_ptr<GraphicTask> graphicTask() override;

    std::vector<GLuint> shaders() const override;

    GLuint setProgramResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const override;


private:
    std::shared_ptr<Texture> _texture;

    class Task : public GraphicTask
    {
    public:
        Task(const std::shared_ptr<Texture>& texture);

        bool defineResources(GraphicContext& context) override;

        GLuint shader() const { return _shaderId; }

        GLuint texture() const { return _textureId; }

    private:
        std::shared_ptr<Texture> _texture;
        GLuint _shaderId;
        GLuint _textureId;
    };

    std::shared_ptr<GraphicTask> _task;
};


class PhysicalSky : public Sky
{
public:
    PhysicalSky();
    virtual ~PhysicalSky();

    std::shared_ptr<GraphicTask> graphicTask() override;

    std::vector<GLuint> shaders() const override;

    GLuint setProgramResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const override;


private:
    typedef atmosphere::Model Model;
    typedef atmosphere::reference::AtmosphereParameters Params;

    std::unique_ptr<Model> _model;
    std::unique_ptr<Params> _params;

    class Task : public GraphicTask
    {
    public:
        Task(Model& model, Params& params);

        bool defineResources(GraphicContext& context) override;

        GLuint shader() const { return _shaderId; }

    private:
        Model& _model;
        Params& _params;
        GLuint _shaderId;
    };

    std::shared_ptr<GraphicTask> _task;
};

}

#endif // SKY_H
