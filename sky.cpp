#include "sky.h"

#include <GLM/gtc/constants.hpp>
#include <GLM/gtx/transform.hpp>
#include <GLM/gtx/quaternion.hpp>

#include "camera.h"
#include "material.h"
#include "scene.h"
#include "body.h"
#include "units.h"
#include "profiler.h"

#include <bruneton/model.h>
#include <bruneton/definitions.h>

using namespace atmosphere::reference;

namespace unisim
{

DeclareProfilePoint(PhysicalSky);
DeclareProfilePointGpu(PhysicalSky);

DefineResource(SkyMap);
DefineResource(MoonAlbedo);
DefineResource(MoonLighting);


DirectionalLight::DirectionalLight(const std::string& name) :
    _name(name),
    _emissionColor(1, 1, 1),
    _emissionLuminance(1)
{

}

DirectionalLight::~DirectionalLight()
{

}

std::shared_ptr<DirectionalLight> makeDirLight(const std::string& name, const glm::dvec3& position, const glm::dvec3& emission, double luminance, double solidAngle)
{
    std::shared_ptr<DirectionalLight> light(new DirectionalLight(name));

    light->setDirection(glm::normalize(position));
    light->setEmissionColor(emission);
    light->setEmissionLuminance(luminance);
    light->setSolidAngle(solidAngle);

    return light;
}

SkyLocalization::SkyLocalization() :
    _longitude(-73.5674f),
    _latitude(45.508888f),
    _timeOfDay(8.0f),
    _dayOfYear(81),
    _dayOfMoon(20)
{
    _sun.reset(new Body(  696.340e6,      1.41f));
    _earth.reset(new Body(EARTH_RADIUS,   5.51f));
    _moon.reset(new Body( 1.738e6,        3.344));
}

void SkyLocalization::computeSunAndMoon(
        glm::vec3& sunDirection,
        glm::vec3& moonDirection,
        glm::vec4& moonQuaternion,
        glm::vec4& starsQuaternion) const
{
    double secSinceBeginningOfYear = _dayOfYear * 24 * 60 * 60;
    _earth->setupOrbit(1.000,  0.017, 0.0, 102.9, 100.5, 0.00, secSinceBeginningOfYear, _sun.get());
    double sunPhase = glm::atan(_earth->position().y, _earth->position().x);

    double earthRotRadians = 360 * _timeOfDay / MAX_TIME_OF_DAY - _longitude + glm::degrees(sunPhase);
    _earth->setupRotation(0.99, earthRotRadians, 000.00, 90.00);

    double moonSecSinceAlignSun = _dayOfMoon * 24 * 60 * 60;
    double moonMeanLongitude = 180 + 0 + glm::degrees(sunPhase);
    _moon->setupOrbit( 0.00257, 0.0554, 125.08, 83.23, moonMeanLongitude, 5.16, moonSecSinceAlignSun, _earth.get());

    glm::dvec4 geoQuat = quatMul(
                quat(glm::dvec3(0, 0, 1), glm::radians(_longitude)),
                quat(glm::dvec3(0, -1, 0), glm::radians(_latitude)));
    glm::dvec4 spaceQuat = quatMul(
                _earth->quaternion(),
                geoQuat);

    glm::dvec3 zenithInSpace = rotatePoint(spaceQuat, glm::dvec3(1, 0, 0));
    glm::dvec3 eastInSpace = rotatePoint(spaceQuat, glm::dvec3(0, 1, 0));
    glm::dvec3 northInSpace = rotatePoint(spaceQuat, glm::dvec3(0, 0, 1));

    glm::dvec3 positionInSpace = _earth->position() + zenithInSpace * double(EARTH_RADIUS);
    glm::dvec3 sunDir(
        glm::dot(-positionInSpace, eastInSpace),
        glm::dot(-positionInSpace, northInSpace),
        glm::dot(-positionInSpace, zenithInSpace));

    sunDirection = glm::normalize(sunDir);

    glm::dvec3 moonRelativePosition = _moon->position() - positionInSpace;
    glm::dvec3 moonDir(
        glm::dot(moonRelativePosition, eastInSpace),
        glm::dot(moonRelativePosition, northInSpace),
        glm::dot(moonRelativePosition, zenithInSpace));

    moonDirection = glm::normalize(moonDir);

    glm::dvec4 earthToSpaceAxisTransform =
        quatMul(quat(glm::dvec3(0, 0, 1), glm::radians(90.0f)),
                quat(glm::dvec3(1, 0, 0), glm::radians(90.0f)));

    glm::vec3 moonRelativeDirection = glm::normalize(moonRelativePosition);
    float moonEclipticLongitude = glm::atan(moonRelativeDirection.y, moonRelativeDirection.x);
    float moonEclipticLatitude = glm::asin(moonRelativeDirection.z);

    moonQuaternion = quatMul(quatMul(quatMul(quatMul(quatMul(
                quat(glm::dvec3(0, 0, 1), glm::radians(145.0f)),
                quat(glm::dvec3(0, 1, 0), glm::radians(90.0f))),
                quat(glm::dvec3(0, 1, 0), moonEclipticLatitude)),
                quat(glm::dvec3(0, 0, 1), -moonEclipticLongitude)),
                spaceQuat), earthToSpaceAxisTransform);

    starsQuaternion = quatMul(quatConjugate(EARTH_BASE_QUAT),
                quatMul(spaceQuat, earthToSpaceAxisTransform));
}

Sky::Sky() :
    _quaternion(0, 0, 0, 1),
    _exposure(1)
{

}

Sky::~Sky()
{

}

SkySphere::SkySphere() :
    Sky()
{
    _texture.reset(new Texture(Texture::BLACK_Float32));
    _task = std::make_shared<SkySphere::Task>(_texture);
}

SkySphere::SkySphere(const std::string& fileName) :
    Sky()
{
    _texture.reset(Texture::load(fileName));
    _task = std::make_shared<SkySphere::Task>(_texture);
}

std::shared_ptr<GraphicTask> SkySphere::graphicTask()
{
    return _task;
}

SkySphere::Task::Task(const std::shared_ptr<Texture>& texture) :
    GraphicTask("SkySphere"),
    _texture(texture)
{

}

bool SkySphere::Task::definePathTracerModules(GraphicContext& context)
{
    GLuint moduleId = 0;
    if(!addPathTracerModule(moduleId, context.settings, "shaders/sphericalsky.glsl"))
        return false;

    return true;
}

bool SkySphere::Task::defineResources(GraphicContext& context)
{
    bool ok = true;

    ResourceManager& resources = context.resources;
    ok = ok && resources.define<GpuTextureResource>(ResourceName(SkyMap), {*_texture});

    _hash = toGpu(context);

    return ok;
}

void SkySphere::Task::setPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const
{
    SkySphere& sky = *dynamic_cast<SkySphere*>(context.scene.sky().get());
    ResourceManager& resources = context.resources;

    GLuint skyMapUnit = interface.grabTextureUnit();
    glActiveTexture(GL_TEXTURE0 + skyMapUnit);
    glBindTexture(GL_TEXTURE_2D, resources.get<GpuTextureResource>(ResourceName(SkyMap)).texId);
    glUniform1i(glGetUniformLocation(interface.programId(), "skyMap"), skyMapUnit);

    glm::vec4 quaternion = sky.quaternion();
    glUniform4fv(glGetUniformLocation(interface.programId(), "skyQuaternion"), 1, &quaternion[0]);

    glUniform1f(glGetUniformLocation(interface.programId(), "skyExposure"), sky.exposure());
}

void SkySphere::Task::update(GraphicContext& context)
{
    _hash = toGpu(context);
}

u_int64_t SkySphere::Task::toGpu(
        const GraphicContext& context) const
{
    const SkySphere& sky = static_cast<SkySphere&>(*context.scene.sky());

    uint64_t hash = 0;
    hash = hashVal(sky.quaternion(), hash);
    hash = hashVal(sky.exposure(), hash);
    return hash;
}

/*
<p>Our tests use length values expressed in kilometers and, for the tests based
on radiance values (as opposed to luminance values), make use of the following
3 wavelengths:
*/

constexpr Length kLengthUnit = 1.0 * km;
constexpr Wavelength kLambdaR = atmosphere::Model::kLambdaR * nm;
constexpr Wavelength kLambdaG = atmosphere::Model::kLambdaG * nm;
constexpr Wavelength kLambdaB = atmosphere::Model::kLambdaB * nm;

/*
<p>where the ground and sphere albedos are provided by the following methods:
*/

PhysicalSky::PhysicalSky() :
    Sky()
{
    bool half_precision = false;
    bool combine_textures = false;
    bool precomputed_luminance = true;

    // Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
    // (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
    // summed and averaged in each bin (e.g. the value for 360nm is the average
    // of the ASTM G-173 values for all wavelengths between 360 and 370nm).
    // Values in W.m^-2.
    constexpr int kLambdaMin = 360;
    constexpr int kLambdaMax = 830;
    constexpr double kSolarIrradiance[48] = {
      1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.6887, 1.61253,
      1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
      1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533,
      1.6965, 1.68194, 1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482,
      1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.2367, 1.2082,
      1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992
    };
    constexpr ScatteringCoefficient kRayleigh = 1.24062e-6 / m;
    constexpr Length kRayleighScaleHeight = 8000.0 * m;
    constexpr Length kMieScaleHeight = 1200.0 * m;
    constexpr double kMieAngstromAlpha = 0.0;
    constexpr double kMieAngstromBeta = 5.328e-3;
    constexpr double kMieSingleScatteringAlbedo = 0.9;
    constexpr double kMiePhaseFunctionG = 0.8;
    // Values from http://www.iup.uni-bremen.de/gruppen/molspec/databases/
    // referencespectra/o3spectra2011/index.html for 233K, summed and averaged
    // in each bin (e.g. the value for 360nm is the average of the original
    // values for all wavelengths between 360 and 370nm). Values in m^2.
    constexpr double kOzoneCrossSection[48] = {
      1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
      8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26,
      9.016e-26, 1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25,
      4.266e-25, 4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25,
      3.74e-25, 3.215e-25, 2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25,
      1.209e-25, 9.423e-26, 7.455e-26, 6.566e-26, 5.105e-26, 4.15e-26,
      4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26, 2.534e-26, 1.624e-26,
      1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
    };
    // From https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
    constexpr dimensional::Scalar<-2, 0, 0, 0, 0> kDobsonUnit = 2.687e20 / m2;
    // Maximum number density of ozone molecules, in m^-3 (computed so at to get
    // 300 Dobson units of ozone - for this we divide 300 DU by the integral of
    // the ozone density profile defined below, which is equal to 15km).
    constexpr NumberDensity kMaxOzoneNumberDensity =
        300.0 * kDobsonUnit / (15.0 * km);

    std::vector<SpectralIrradiance> solar_irradiance;
    std::vector<ScatteringCoefficient> rayleigh_scattering;
    std::vector<ScatteringCoefficient> mie_scattering;
    std::vector<ScatteringCoefficient> mie_extinction;
    std::vector<ScatteringCoefficient> absorption_extinction;
    for (int l = kLambdaMin; l <= kLambdaMax; l += 10) {
      double lambda = static_cast<double>(l) * 1e-3;  // micro-meters
      SpectralIrradiance solar = kSolarIrradiance[(l - kLambdaMin) / 10] *
          watt_per_square_meter_per_nm;
      ScatteringCoefficient rayleigh = kRayleigh * pow(lambda, -4);
      ScatteringCoefficient mie = kMieAngstromBeta / kMieScaleHeight *
          pow(lambda, -kMieAngstromAlpha);
      solar_irradiance.push_back(solar);
      rayleigh_scattering.push_back(rayleigh);
      mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
      mie_extinction.push_back(mie);
      absorption_extinction.push_back(kMaxOzoneNumberDensity *
          kOzoneCrossSection[(l - kLambdaMin) / 10] * m2);
    }

    AtmosphereParameters atmosphere_parameters_;
    atmosphere_parameters_.solar_irradiance = IrradianceSpectrum(
        kLambdaMin * nm, kLambdaMax * nm, solar_irradiance);
    atmosphere_parameters_.sun_angular_radius = 0.2678 * deg;
    atmosphere_parameters_.bottom_radius = 6360.0 * km;
    atmosphere_parameters_.top_radius = 6420.0 * km;
    atmosphere_parameters_.rayleigh_density.layers[1] = DensityProfileLayer(
        0.0 * m, 1.0, -1.0 / kRayleighScaleHeight, 0.0 / m, 0.0);
    atmosphere_parameters_.rayleigh_scattering = ScatteringSpectrum(
        kLambdaMin * nm, kLambdaMax * nm, rayleigh_scattering);
    atmosphere_parameters_.mie_density.layers[1] = DensityProfileLayer(
        0.0 * m, 1.0, -1.0 / kMieScaleHeight, 0.0 / m, 0.0);
    atmosphere_parameters_.mie_scattering = ScatteringSpectrum(
        kLambdaMin * nm, kLambdaMax * nm, mie_scattering);
    atmosphere_parameters_.mie_extinction = ScatteringSpectrum(
        kLambdaMin * nm, kLambdaMax * nm, mie_extinction);
    atmosphere_parameters_.mie_phase_function_g = kMiePhaseFunctionG;
    // Density profile increasing linearly from 0 to 1 between 10 and 25km, and
    // decreasing linearly from 1 to 0 between 25 and 40km. Approximate profile
    // from http://www.kln.ac.lk/science/Chemistry/Teaching_Resources/Documents/
    // Introduction%20to%20atmospheric%20chemistry.pdf (page 10).
    atmosphere_parameters_.absorption_density.layers[0] = DensityProfileLayer(
        25.0 * km, 0.0, 0.0 / km, 1.0 / (15.0 * km), -2.0 / 3.0);
    atmosphere_parameters_.absorption_density.layers[1] = DensityProfileLayer(
        0.0 * km, 0.0, 0.0 / km, -1.0 / (15.0 * km), 8.0 / 3.0);
    atmosphere_parameters_.absorption_extinction = ScatteringSpectrum(
        kLambdaMin * nm, kLambdaMax * nm, absorption_extinction);
    atmosphere_parameters_.ground_albedo = DimensionlessSpectrum(0.18);
    atmosphere_parameters_.mu_s_min = cos(102.0 * deg);

    Position earth_center_ =
        Position(0.0 * m, 0.0 * m, -atmosphere_parameters_.bottom_radius);

    dimensional::vec2 sun_size_ = dimensional::vec2(
        tan(atmosphere_parameters_.sun_angular_radius),
        cos(atmosphere_parameters_.sun_angular_radius));

    std::vector<double> wavelengths;
    const auto& spectrum = atmosphere_parameters_.solar_irradiance;
    for (unsigned int i = 0; i < spectrum.size(); ++i) {
      wavelengths.push_back(spectrum.GetSample(i).to(nm));
    }
    auto profile = [](DensityProfileLayer layer) {
      return atmosphere::DensityProfileLayer(layer.width.to(m),
          layer.exp_term(), layer.exp_scale.to(1.0 / m),
          layer.linear_term.to(1.0 / m), layer.constant_term());
    };

    _params.reset(new Params(atmosphere_parameters_));

    _model.reset(new atmosphere::Model(
         wavelengths,
         atmosphere_parameters_.solar_irradiance.to(
             watt_per_square_meter_per_nm),
         atmosphere_parameters_.sun_angular_radius.to(rad),
         atmosphere_parameters_.bottom_radius.to(m),
         atmosphere_parameters_.top_radius.to(m),
         {profile(atmosphere_parameters_.rayleigh_density.layers[1])},
         atmosphere_parameters_.rayleigh_scattering.to(1.0 / m),
         {profile(atmosphere_parameters_.mie_density.layers[1])},
         atmosphere_parameters_.mie_scattering.to(1.0 / m),
         atmosphere_parameters_.mie_extinction.to(1.0 / m),
         atmosphere_parameters_.mie_phase_function_g(),
         {profile(atmosphere_parameters_.absorption_density.layers[0]),
          profile(atmosphere_parameters_.absorption_density.layers[1])},
         atmosphere_parameters_.absorption_extinction.to(1.0 / m),
         atmosphere_parameters_.ground_albedo.to(Number::Unit()),
         acos(atmosphere_parameters_.mu_s_min()),
         kLengthUnit.to(m),
         precomputed_luminance ? 15 : 3 /* num_computed_wavelengths */,
         combine_textures,
         half_precision));

    _model->Init();

    glm::dvec3 sunIrradiance;
    Model::ConvertSpectrumToLinearSrgb(wavelengths,
        atmosphere_parameters_.solar_irradiance.to(watt_per_square_meter_per_nm),
        &sunIrradiance[0], &sunIrradiance[1], &sunIrradiance[2]);
    _sunIrradiance = sunIrradiance;

    float sunSolidAngle = 2 * glm::pi<float>() *(1 - glm::cos(atmosphere_parameters_.sun_angular_radius.to(rad)));
    float maxComp = glm::max(glm::max(_sunIrradiance[0], _sunIrradiance[1]), _sunIrradiance[2]);
    _sunIrradiance /= maxComp;
    float luminance = maxComp / sunSolidAngle;

    _sun = makeDirLight("Sun", glm::vec3(0, 0, 1), _sunIrradiance, luminance, sunSolidAngle);
    directionalLights().push_back(_sun);

    _moon = makeDirLight("Moon", glm::vec3(0, 0, 1), _sunIrradiance, 0.0f, sunSolidAngle);
    directionalLights().push_back(_moon);

    _starsTexture.reset(Texture::load("textures/starmap_2020_4k.exr"));

    _task = std::make_shared<PhysicalSky::Task>(*_model, *_params, *_sun, *_moon, _starsTexture);
}

PhysicalSky::~PhysicalSky()
{

}

std::shared_ptr<GraphicTask> PhysicalSky::graphicTask()
{
    return _task;
}

struct PhysicalSkyCommonParams
{
    glm::mat4 transform;
    glm::vec4 sunDirection;
    glm::vec4 moonDirection;
    glm::vec4 sunLi;
};

PhysicalSky::Task::Task(
        Model& model,
        Params& params,
        DirectionalLight& sun,
        DirectionalLight& moon,
        const std::shared_ptr<Texture>& stars) :
    GraphicTask("PhysicalSphere"),
    _model(model),
    _params(params),
    _sun(sun),
    _moon(moon),
    _lightingProgramId(0),
    _moonLightingDimensions(1024),
    _moonHash(0),
    _moonIsDirty(true),
    _starsTexture(stars)
{

}

bool PhysicalSky::Task::definePathTracerModules(GraphicContext& context)
{
    GLuint moduleId = 0;
    if(!addPathTracerModule(moduleId, context.settings, "shaders/physicalsky.glsl"))
        return false;

    addPathTracerModule(_model.shader());

    return true;
}

bool PhysicalSky::Task::defineResources(GraphicContext& context)
{
    bool ok = true;

    ResourceManager& resources = context.resources;

    if(!generateComputeProgram(_lightingProgramId, "shaders/moonlight.glsl"))
        return false;

    _moonAlbedo.reset(Texture::load("textures/moonAlbedo2d.png"));
    ok = ok && resources.define<GpuTextureResource>(ResourceName(MoonAlbedo), {*_moonAlbedo});

    _lightingFormat = GL_RGBA32F;
    ok = ok && resources.define<GpuImageResource>(ResourceName(MoonLighting),
        {_moonLightingDimensions, _moonLightingDimensions, _lightingFormat});

    _albedoLoc = glGetUniformLocation(_lightingProgramId, "albedo");
    _lightingLoc = glGetUniformLocation(_lightingProgramId, "lighting");

    _albedoUnit = 0;
    _lightingUnit = 0;

    glProgramUniform1i(_lightingProgramId, _albedoLoc, _albedoUnit);
    glProgramUniform1i(_lightingProgramId, _lightingLoc, _lightingUnit);

    glGenBuffers(1, &_paramsUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, _paramsUbo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PhysicalSkyCommonParams), nullptr, GL_STREAM_DRAW);

    ok = ok && resources.define<GpuTextureResource>(ResourceName(SkyMap), {*_starsTexture});

    _gpuParams.reset(new PhysicalSkyCommonParams());
    _hash = toGpu(context, *_gpuParams);
    _moonIsDirty = true;

    return ok;
}

void PhysicalSky::Task::setPathTracerResources(GraphicContext& context, PathTracerInterface& interface) const
{
    PhysicalSky& sky = *dynamic_cast<PhysicalSky*>(context.scene.sky().get());
    ResourceManager& resources = context.resources;

    GLuint textureUnits[] = {
        interface.grabTextureUnit(),
        interface.grabTextureUnit(),
        interface.grabTextureUnit(),
        interface.grabTextureUnit(),
    };

   _model.SetProgramUniforms(interface.programId(), textureUnits[0], textureUnits[1], textureUnits[2], textureUnits[3]);

    glm::vec3 sunDirection = _sun.direction();
    glm::vec3 moonDirection = _moon.direction();
    float sunToMoonRatio = _moon.emissionLuminance() / _sun.emissionLuminance();

    glUniform3f(glGetUniformLocation(interface.programId(), "sunDirection"),
                sunDirection[0], sunDirection[1], sunDirection[2]);

    glUniform3f(glGetUniformLocation(interface.programId(), "moonDirection"),
                moonDirection[0], moonDirection[1], moonDirection[2]);

    glUniform1f(glGetUniformLocation(interface.programId(), "sunToMoonRatio"),
                sunToMoonRatio);

    glUniform1f(glGetUniformLocation(interface.programId(), "groundHeightKM"),
                _params.bottom_radius.to(km));

    GLuint moonLightingUnit = interface.grabTextureUnit();
    glActiveTexture(GL_TEXTURE0 + moonLightingUnit);
    glBindTexture(GL_TEXTURE_2D, resources.get<GpuImageResource>(ResourceName(MoonLighting)).texId);
    glUniform1i(glGetUniformLocation(interface.programId(), "moon"), moonLightingUnit);

    glUniform4fv(glGetUniformLocation(interface.programId(), "moonQuaternion"), 1, &_moonQuaternion[0]);

    GLuint starsUnit = interface.grabTextureUnit();
    glActiveTexture(GL_TEXTURE0 + starsUnit);
    glBindTexture(GL_TEXTURE_2D, resources.get<GpuTextureResource>(ResourceName(SkyMap)).texId);
    glUniform1i(glGetUniformLocation(interface.programId(), "stars"), starsUnit);

    glm::vec4 starsQuaternion = sky.quaternion();
    glUniform4fv(glGetUniformLocation(interface.programId(), "starsQuaternion"), 1, &starsQuaternion[0]);

    glUniform1f(glGetUniformLocation(interface.programId(), "starsExposure"), sky.exposure());
}

void PhysicalSky::Task::update(GraphicContext& context)
{
    Profile(PhysicalSky);

    glm::vec4 starsQuaternion;
    glm::vec3 sunDirection, moonDirection, moonUp;
    context.scene.sky()->localization().computeSunAndMoon(sunDirection, moonDirection, _moonQuaternion, starsQuaternion);

    _sun.setDirection(sunDirection);
    _moon.setDirection(moonDirection);
    context.scene.sky()->setQuaternion(starsQuaternion);

    // Moon transform
    _moonTransform = quatMat4(quatConjugate(_moonQuaternion));

    // Moon luminance approximation (hexa-web sampling)
    float maxMoonAlbedo = 0.14;
    float L_i = _sun.emissionLuminance() * _sun.solidAngle() / glm::pi<float>();

    glm::vec2 luminanceAvg(0, 0);
    int RADIUS_SAMP_COUNT = 16;
    for(int r_i = 0; r_i < RADIUS_SAMP_COUNT; ++r_i)
    {
        float r = float(r_i + 1) / RADIUS_SAMP_COUNT;

        int phi_samp_count = (r_i + 1) * 6;
        for(int p_i = 0; p_i < phi_samp_count; ++p_i)
        {
            float phi = 2 * glm::pi<float>() * float(p_i) / phi_samp_count;
            glm::vec2 pos = glm::vec2();

            // Orthographic mapping
            float dir_z = glm::sqrt(1 - r*r);
            glm::vec3 dir_orth(glm::cos(phi)*r, glm::sin(phi)*r, dir_z);
            glm::vec3 dir_earth = glm::vec3(_moonTransform * glm::vec4(dir_orth, 0));

            float backScattering = 2 + glm::dot(-moonDirection, sunDirection) / 3.0f;
            float L_o = maxMoonAlbedo * L_i * glm::max(0.0f, glm::dot(dir_earth, sunDirection)) * backScattering;
             luminanceAvg += glm::vec2(L_o, 1.0f);
        }
    }

    float moonLuminance = luminanceAvg.x / glm::max(1e-5f, luminanceAvg.y);
    _moon.setEmissionLuminance(moonLuminance);

    uint64_t hash = toGpu(context, *_gpuParams);
    _moonIsDirty = hash != _hash;
    _hash = hash;
}

void PhysicalSky::Task::render(GraphicContext& context)
{
    ResourceManager& resources = context.resources;

    ProfileGpu(PhysicalSky);

    if(!_moonIsDirty)
        return;

    glUseProgram(_lightingProgramId);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, _paramsUbo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PhysicalSkyCommonParams), _gpuParams.get(), GL_STREAM_DRAW);

    glActiveTexture(GL_TEXTURE0 + _albedoUnit);
    glBindTexture(GL_TEXTURE_2D, resources.get<GpuTextureResource>(ResourceName(MoonAlbedo)).texId);

    glBindImageTexture(_lightingUnit, resources.get<GpuImageResource>(ResourceName(MoonLighting)).texId, 0, false, 0, GL_WRITE_ONLY, _lightingFormat);

    glDispatchCompute((_moonLightingDimensions) / 8, _moonLightingDimensions / 8, 1);

    _moonIsDirty = false;
}

uint64_t PhysicalSky::Task::toGpu(
        const GraphicContext& context,
        PhysicalSkyCommonParams& params) const
{
    const PhysicalSky& sky = static_cast<PhysicalSky&>(*context.scene.sky());

    params.transform = _moonTransform;
    params.sunDirection = glm::vec4(_sun.direction(), 0);
    params.moonDirection = glm::vec4(_moon.direction(), 0);
    params.sunLi = glm::vec4(_sun.emissionColor() * _sun.emissionLuminance() * _sun.solidAngle() / glm::pi<float>(), 0);

    uint64_t hash = 0;
    hash = hashVal(params, hash);
    hash = hashVal(sky.quaternion(), hash);
    hash = hashVal(sky.exposure(), hash);

    return hash;
}

}
