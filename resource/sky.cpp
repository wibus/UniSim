#include "sky.h"

#include <imgui/imgui.h>

#include <bruneton/model.h>
#include <bruneton/definitions.h>

#include "../system/units.h"

#include "../resource/body.h"
#include "../resource/light.h"
#include "../resource/texture.h"


using namespace atmosphere::reference;


namespace unisim
{
constexpr Length kLengthUnit = 1.0 * km;
constexpr Wavelength kLambdaR = atmosphere::Model::kLambdaR * nm;
constexpr Wavelength kLambdaG = atmosphere::Model::kLambdaG * nm;
constexpr Wavelength kLambdaB = atmosphere::Model::kLambdaB * nm;


// SKY LOCALIZATION //

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

void SkyLocalization::ui()
{
    glm::vec2 geom(_longitude, _latitude);
    if(ImGui::InputFloat2("Long/Lat", &geom[0]))
    {
        setLongitude(geom[0]);
        setLatitude(geom[1]);
    }

    float timeOfDay = _timeOfDay;
    if(ImGui::SliderFloat("Time of Day", &timeOfDay, 0, SkyLocalization::MAX_TIME_OF_DAY))
        setTimeOfDay(timeOfDay);

    float dayOfYear = _dayOfYear;
    if(ImGui::SliderFloat("Day of Year", &dayOfYear, 0, SkyLocalization::MAX_DAY_OF_YEAR))
        setDayOfYear(dayOfYear);

    float dayOfMoon = _dayOfMoon;
    if(ImGui::SliderFloat("Day of Moon", &dayOfMoon, 0, SkyLocalization::MAX_DAY_OF_MOON))
        setDayOfMoon(dayOfMoon);
}


// STARS //

Stars::Stars() :
    _starsTexture(Texture::load("textures/starmap_2020_4k.exr")),
    _starsQuaternion(EARTH_BASE_QUAT),
    _starsExposure(1.0f)
{

}

void Stars::ui()
{
    glm::vec4 quaternion = _starsQuaternion;
    if(ImGui::InputFloat4("Quaternion", &quaternion[0]))
    {
        quaternion = glm::normalize(quaternion);
        setQuaternion(quaternion);
    }

    float exposure = _starsExposure;
    float ev = glm::log2(exposure);
    if(ImGui::SliderFloat("Exposure", &ev, -20, 20))
    {
        exposure = glm::pow(2.0f, ev);
        setExposure(exposure);
    }

    if (Texture* stars = _starsTexture.get())
    {
        stars->ui();
    }
}


// PHYSICAL SKY //

Atmosphere::Atmosphere()
{
    bool precomputed_luminance = false;

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
        precomputed_luminance ? 15 : 3 /* num_computed_wavelengths */));

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

    auto makeDirLight = [](const std::string& name, const glm::dvec3& position, const glm::dvec3& emission, double luminance, double solidAngle)
    {
        std::shared_ptr<DirectionalLight> light(new DirectionalLight(name));

        light->setDirection(glm::normalize(position));
        light->setEmissionColor(emission);
        light->setEmissionLuminance(luminance);
        light->setSolidAngle(solidAngle);

        return light;
    };

    _sun = makeDirLight("Sun", glm::vec3(0, 0, 1), _sunIrradiance, luminance, sunSolidAngle);
    _moon = makeDirLight("Moon", glm::vec3(0, 0, 1), _sunIrradiance, 0.0f, sunSolidAngle);
}

Atmosphere::~Atmosphere()
{

}

void Atmosphere::ui()
{
    if(ImGui::TreeNode(_sun->name().c_str()))
    {
        _sun->ui();
        ImGui::TreePop();
    }

    if(ImGui::TreeNode(_moon->name().c_str()))
    {
        _moon->ui();
        ImGui::TreePop();
    }
}


// SKY //

Sky::Sky(
    const std::shared_ptr<Stars>& stars,
    const std::shared_ptr<Atmosphere>& atmosphere) :
    _stars(stars),
    _atmosphere(atmosphere)
{
    if(!_stars)
    {
        _stars.reset(new Stars());
    }
}

Sky::~Sky()
{

}

void Sky::ui()
{
    _localization.ui();

    if (_stars)
    {
        ImGui::Separator();

        _stars->ui();
    }

    if (_atmosphere)
    {
        ImGui::Separator();

        _atmosphere->ui();
    }
}


}
