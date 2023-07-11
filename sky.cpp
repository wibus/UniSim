#include "sky.h"

#include "camera.h"
#include "material.h"
#include "scene.h"

#include <bruneton/model.h>
#include <bruneton/definitions.h>

using namespace atmosphere::reference;

namespace unisim
{

DefineResource(SkyMap);

Sky::Sky(Mapping mapping) :
    _mapping(mapping),
    _quaternion(0, 0, 0, 1),
    _exposure(1)
{

}

Sky::~Sky()
{

}

SkySphere::SkySphere() :
    Sky(Mapping::Sphere)
{
    _texture.reset(new Texture(Texture::BLACK_Float32));
    _task = std::make_shared<SkySphere::Task>(_texture);
}

SkySphere::SkySphere(const std::string& fileName) :
    Sky(Mapping::Sphere)
{
    _texture.reset(Texture::load(fileName));
    _task = std::make_shared<SkySphere::Task>(_texture);
}

std::shared_ptr<GraphicTask> SkySphere::graphicTask()
{
    return _task;
}

std::vector<GLuint> SkySphere::shaders() const
{
    return {static_cast<SkySphere::Task&>(*_task).shader()};
}

GLuint SkySphere::setProgramResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const
{
    GLuint texId = static_cast<SkySphere::Task&>(*_task).texture();

    GLuint skyMapUnit = textureUnitStart++;
    glActiveTexture(GL_TEXTURE0 + skyMapUnit);
    glBindTexture(GL_TEXTURE_2D, texId);
    glUniform1i(glGetUniformLocation(programId, "skyMap"), skyMapUnit);

    glUniform4f(glGetUniformLocation(programId, "skyQuaternion"),
                quaternion()[0], quaternion()[1], quaternion()[2], quaternion()[3]);

    glUniform1f(glGetUniformLocation(programId, "skyExposure"), exposure());

    return textureUnitStart;
}

SkySphere::Task::Task(const std::shared_ptr<Texture>& texture) :
    GraphicTask("SkySphere"),
    _texture(texture)
{

}

bool SkySphere::Task::defineResources(GraphicContext& context)
{
    ResourceManager& resources = context.resources;
    resources.define<GpuTextureResource>(ResourceName(SkyMap), {*_texture});
    _textureId = resources.get<GpuTextureResource>(ResourceName(SkyMap)).texId;

    if(!generateComputerShader(_shaderId, "shaders/sphericalsky.glsl"))
        return false;

    return true;
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
    Sky(Mapping::Sphere)
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

    _task = std::make_shared<PhysicalSky::Task>(*_model, *_params);
}

PhysicalSky::~PhysicalSky()
{

}

std::shared_ptr<GraphicTask> PhysicalSky::graphicTask()
{
    return _task;
}

std::vector<GLuint> PhysicalSky::shaders() const
{
    return {static_cast<PhysicalSky::Task&>(*_task).shader(), _model->shader()};
}

GLuint PhysicalSky::setProgramResources(GraphicContext& context, GLuint programId, GLuint textureUnitStart) const
{
    GLuint textureUnits[] = {
        textureUnitStart++,
        textureUnitStart++,
        textureUnitStart++,
        textureUnitStart++,
    };

    _model->SetProgramUniforms(programId, textureUnits[0], textureUnits[1], textureUnits[2], textureUnits[3]);

    glm::vec3 sunDirection(0, 0, 1);
    if(context.scene.directionalLights().size() > 0)
    {
        sunDirection = context.scene.directionalLights()[0]->direction();
    }

    glUniform3f(glGetUniformLocation(programId, "sunDirection"),
                sunDirection[0], sunDirection[1], sunDirection[2]);

    glUniform1f(glGetUniformLocation(programId, "groundHeightKM"),
                _params->bottom_radius.to(km));

    return textureUnitStart;
}

PhysicalSky::Task::Task(Model& model, Params& params) :
    GraphicTask("PhysicalSphere"),
    _model(model),
    _params(params),
    _shaderId(0)
{

}

bool PhysicalSky::Task::defineResources(GraphicContext& context)
{
    if(!generateComputerShader(_shaderId, "shaders/physicalsky.glsl"))
        return false;

    return true;
}

}
