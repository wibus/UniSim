#ifndef SKY_H
#define SKY_H

#include <memory>

#include <GLM/glm.hpp>


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

class Body;
class Texture;
class DirectionalLight;


class SkyLocalization
{
public:
    SkyLocalization();

    static constexpr float MAX_TIME_OF_DAY = 24.0f;
    static constexpr float MAX_DAY_OF_YEAR = 365.2422f;
    static constexpr float MAX_DAY_OF_MOON = 27.322f;

    static constexpr float EARTH_RADIUS = 6.371e6f;

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

    void ui();

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


class Stars
{
public:
    Stars();

    glm::vec4 starsQuaternion() const { return _starsQuaternion; }
    void setQuaternion(const glm::vec4& quaternion) { _starsQuaternion = quaternion; }

    float starsExposure() const { return _starsExposure; }
    void setExposure(float exposure) { _starsExposure = exposure; }

    std::shared_ptr<Texture> starsTexture() const { return _starsTexture; }

    void ui();

private:
    std::shared_ptr<Texture> _starsTexture;
    glm::vec4 _starsQuaternion;
    float _starsExposure;
};


class Atmosphere
{
public:
    typedef atmosphere::Model Model;
    typedef atmosphere::reference::AtmosphereParameters Params;

    Atmosphere();
    ~Atmosphere();

    const Model& model() const {return *_model; }
    const Params& params() const { return *_params; }

    std::shared_ptr<DirectionalLight> sun() const { return _sun; }
    std::shared_ptr<DirectionalLight> moon() const  { return _moon; }

    void ui();

private:
    std::unique_ptr<Model> _model;
    std::unique_ptr<Params> _params;

    glm::vec3 _sunIrradiance;
    std::shared_ptr<DirectionalLight> _sun;
    std::shared_ptr<DirectionalLight> _moon;
};


class Sky
{
public:
    Sky(const std::shared_ptr<Stars>& stars = nullptr,
        const std::shared_ptr<Atmosphere>& atmosphere = nullptr);
    ~Sky();

    SkyLocalization& localization() { return _localization; }
    const SkyLocalization& localization() const { return _localization; }

    std::shared_ptr<Stars> stars() const { return _stars; }

    std::shared_ptr<Atmosphere> atmosphere() const { return _atmosphere; }

    void ui();

private:
    SkyLocalization _localization;

    std::shared_ptr<Stars> _stars;
    std::shared_ptr<Atmosphere> _atmosphere;
};


}

#endif // SKY_H
