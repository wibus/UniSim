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
class Body;


class DirectionalLight
{
public:
    DirectionalLight(const std::string& name);
    ~DirectionalLight();

    const std::string& name() const { return _name; }

    glm::vec3 direction() const { return _position; }
    void setDirection(const glm::vec3& position) { _position = position; }

    glm::vec3 emissionColor() const { return _emissionColor; }
    void setEmissionColor(const glm::vec3& color) { _emissionColor = color; }

    float emissionLuminance() const { return _emissionLuminance; }
    void setEmissionLuminance(float luminance) { _emissionLuminance = luminance; }

    float solidAngle() const { return _solidAngle; }
    void setSolidAngle(float solidAngle) { _solidAngle = solidAngle; }

private:
    std::string _name;
    glm::vec3 _position;
    glm::vec3 _emissionColor;
    float _emissionLuminance;
    float _solidAngle;
};


class SkyLocalization
{
public:
    SkyLocalization();

    static constexpr float MAX_TIME_OF_DAY = 24.0f;
    static constexpr float MAX_DAY_OF_YEAR = 365.2422f;
    static constexpr float MAX_DAY_OF_MOON = 27.322f;

    float longitude() const { return _longitude; }
    void setLongitude(float longitude) { _longitude = longitude; }

    float latitude() const { return _latitude; };
    void setLatitude(float latitude) { _latitude = latitude; }

    float timeOfDay() const { return _timeOfDay; };
    void setTimeOfDay(float timeOfDay) { _timeOfDay = timeOfDay; }

    float dayOfYear() const { return _dayOfYear; };
    void setDayOfYear(float day) { _dayOfYear = day; }

    float dayOfMoon() const { return _dayOfMoon; };
    void setDayOfMoon(float day) { _dayOfMoon = day; }

    void computeSunAndMoon(
            glm::vec3& sunDirection,
            glm::vec3& moonDirection,
            glm::vec4& moonQuaternion,
            glm::vec4& starsQuaternion) const;

private:
    float _longitude;
    float _latitude;

    float _timeOfDay;
    float _dayOfYear;
    float _dayOfMoon;

    std::unique_ptr<Body> _sun;
    std::unique_ptr<Body> _earth;
    std::unique_ptr<Body> _moon;
};


class Sky
{
public:
    Sky();
    virtual ~Sky();

    SkyLocalization& localization() { return _localization; }
    const SkyLocalization& localization() const { return _localization; }

    glm::vec4 quaternion() const { return _quaternion; }
    void setQuaternion(const glm::vec4& quaternion) { _quaternion = quaternion; }

    float exposure() const { return _exposure; }
    void setExposure(float exposure) { _exposure = exposure; }

    std::vector<std::shared_ptr<DirectionalLight>>& directionalLights() { return _directionalLights; }
    const std::vector<std::shared_ptr<DirectionalLight>>& directionalLights() const { return _directionalLights; }

    virtual std::shared_ptr<GraphicTask> graphicTask() = 0;

    virtual std::vector<GLuint> pathTracerShaders() const = 0;

    virtual GLuint setPathTracerResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const = 0;

private:
    SkyLocalization _localization;
    glm::vec4 _quaternion;
    float _exposure;

    std::vector<std::shared_ptr<DirectionalLight>> _directionalLights;
};


class SkySphere : public Sky
{
public:
    SkySphere();
    SkySphere(const std::string& fileName);

    std::shared_ptr<GraphicTask> graphicTask() override;

    std::vector<GLuint> pathTracerShaders() const override;

    GLuint setPathTracerResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const override;


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

    std::vector<GLuint> pathTracerShaders() const override;

    GLuint setPathTracerResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const override;

private:
    typedef atmosphere::Model Model;
    typedef atmosphere::reference::AtmosphereParameters Params;

    class Task : public GraphicTask
    {
    public:
        Task(
                Model& model,
                Params& params,
                DirectionalLight& sun,
                DirectionalLight& moon,
                const std::shared_ptr<Texture>& stars);

        bool defineResources(GraphicContext& context) override;

        void update(GraphicContext& context) override;
        void render(GraphicContext& context) override;

        GLuint shader() const { return _shaderId; }

        glm::vec4 moonQuaternion() const { return _moonQuaternion; }

    private:
        Model& _model;
        Params& _params;
        DirectionalLight& _sun;
        DirectionalLight& _moon;
        glm::mat4 _moonTransform;
        glm::vec4 _moonQuaternion;

        GLuint _shaderId;
        GLuint _lightingProgramId;

        GLuint _paramsUbo;

        GLuint _albedoLoc;
        GLuint _albedoUnit;

        GLuint _lightingLoc;
        GLuint _lightingUnit;
        GLint _lightingFormat;

        std::unique_ptr<Texture> _moonAlbedo;
        int _moonLightingDimensions;

        std::size_t _lastFrameHash;

        std::shared_ptr<Texture> _starsTexture;
    };

    std::unique_ptr<Model> _model;
    std::unique_ptr<Params> _params;

    glm::vec3 _sunIrradiance;
    std::shared_ptr<DirectionalLight> _sun;
    std::shared_ptr<DirectionalLight> _moon;

    std::shared_ptr<Texture> _starsTexture;

    std::shared_ptr<Task> _task;
};

}

#endif // SKY_H
