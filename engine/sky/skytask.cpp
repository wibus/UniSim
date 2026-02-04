#include "skytask.h"

#include <imgui/imgui.h>

#include <bruneton/model.h>
#include <bruneton/definitions.h>

#include <GLM/gtc/constants.hpp>
#include <GLM/gtx/transform.hpp>
#include <GLM/gtx/quaternion.hpp>

#include "../system/profiler.h"
#include "../system/units.h"

#include "../graphic/graphic.h"
#include "../graphic/gpudevice.h"

#include "../resource/body.h"
#include "../resource/light.h"
#include "../resource/sky.h"
#include "../resource/texture.h"

#include "../scene.h"

namespace unisim
{

DefineProfilePoint(Sky);
DefineProfilePointGpu(Sky);

DefineResource(StarsParams);
DefineResource(AtmosphereParams);
DefineResource(MoonLightParams);
DefineResource(MoonAlbedo);
DefineResource(MoonLighting);

DefineResource(Stars);


struct GpuStarsParams
{
    glm::vec4 starsQuaternion;
    float starsExposure;
};

struct GpuAtmosphereParams
{
    glm::vec4 sunDirection;
    glm::vec4 moonDirection;
    glm::vec4 moonQuaternion;
    float sunToMoonRatio;
    float groundHeightKM;
};

struct GpuMoonLightParams
{
    glm::mat4 transform;
    glm::vec4 sunDirection;
    glm::vec4 moonDirection;
    glm::vec4 sunLi;
};


SkyTask::AtmosphereRenderState::AtmosphereRenderState() :
    _moonTexSize(1024),
    _atmosphereHash(0),
    _moonIsDirty(true)
{

}

SkyTask::AtmosphereRenderState::~AtmosphereRenderState()
{

}


SkyTask::SkyTask() :
    PathTracerProviderTask("Sky")
{
}

bool SkyTask::defineResources(GraphicContext& context)
{
    bool ok = true;

    GpuResourceManager& resources = context.resources;

    GpuStarsParams starsParams;
    _hash = toGpu(context, starsParams);
    ok = ok && resources.define<GpuConstantResource>(ResourceName(StarsParams), {sizeof(starsParams), &starsParams});
    ok = ok && resources.define<GpuTextureResource>(ResourceName(Stars), {*context.scene.sky()->stars()->starsTexture()});

    if (context.scene.sky()->atmosphere().get() != nullptr)
    {
        _atmosphereRenderState.reset(new AtmosphereRenderState());
        _atmosphereRenderState->_moonAlbedo.reset(Texture::load("textures/moonAlbedo2d.png"));
        if(!_atmosphereRenderState->_moonAlbedo)
            _atmosphereRenderState->_moonAlbedo.reset(new Texture(Texture::BLACK_UNORM8));
        _atmosphereRenderState->_moonIsDirty = true;

        GpuMoonLightParams moonLightParams;
        GpuAtmosphereParams atmosphereParams;
        _atmosphereRenderState->_atmosphereHash = toGpu(context, moonLightParams, atmosphereParams);
        _hash = combineHashes(_atmosphereRenderState->_atmosphereHash, _hash);

        ok = ok && resources.define<GpuConstantResource>(ResourceName(AtmosphereParams), {sizeof(atmosphereParams), &atmosphereParams});
        ok = ok && resources.define<GpuConstantResource>(ResourceName(MoonLightParams), {sizeof(moonLightParams), &moonLightParams});
        ok = ok && resources.define<GpuTextureResource>(ResourceName(MoonAlbedo), {*_atmosphereRenderState->_moonAlbedo});
        ok = ok && resources.define<GpuImageResource>(ResourceName(MoonLighting), {_atmosphereRenderState->_moonTexSize, _atmosphereRenderState->_moonTexSize, GL_RGBA32F});
    }

    return ok;
}

bool SkyTask::defineShaders(GraphicContext& context)
{
    if (_atmosphereRenderState)
    {
        _atmosphereRenderState->_moonLightGpi.reset(new GpuProgramInterface());
        _atmosphereRenderState->_moonLightGpi->declareConstant({"MoonLightParams"});
        _atmosphereRenderState->_moonLightGpi->declareTexture({"MoonAlbedo"});
        _atmosphereRenderState->_moonLightGpi->declareImage({"MoonLighting"});

        _atmosphereRenderState->_moonLightProgram.reset();
        if(!generateComputeProgram(_atmosphereRenderState->_moonLightProgram, "Moon Light", "shaders/moonlight.glsl"))
            return false;
    }

    return true;
}

bool SkyTask::definePathTracerModules(
    GraphicContext& context,
    std::vector<std::shared_ptr<PathTracerModule>>& modules)
{
    if (_atmosphereRenderState)
    {
        if(!addPathTracerModule(modules, "Atmospheric Sky", context.settings, "shaders/atmosphericsky.glsl"))
            return false;

        std::shared_ptr<GraphicShader> modelShader(new GraphicShader("Sky Model", std::move(GraphicShaderHandle(context.scene.sky()->atmosphere()->model().shader(), false))));
        std::shared_ptr<PathTracerModule> modelModule(new PathTracerModule("Sky Model", modelShader));
        modules.push_back(modelModule);
    }
    else
    {
        if(!addPathTracerModule(modules, "Outerspace Sky", context.settings, "shaders/outerspacesky.glsl"))
            return false;
    }

    return true;
}

bool SkyTask::definePathTracerInterface(
    GraphicContext& context,
    PathTracerInterface& interface)
{
    bool ok = true;

    ok = ok && interface.declareConstant({"StarsParams"});
    ok = ok && interface.declareTexture({"Stars"});

    if (_atmosphereRenderState)
    {
        ok = ok && interface.declareConstant({"AtmosphereParams"});
        ok = ok && interface.declareTexture({"Moon"});

        // Bruneton
        ok = ok && interface.declareTexture({"transmittance_texture"});
        ok = ok && interface.declareTexture({"scattering_texture"});
        //ok = ok && interface.declareTexture({"irradiance_texture"});
        ok = ok && interface.declareTexture({"single_mie_scattering_texture"});
    }

    return ok;
}

void SkyTask::bindPathTracerResources(
    GraphicContext& context,
    CompiledGpuProgramInterface& compiledGpi) const
{
    GpuResourceManager& resources = context.resources;

    context.device.bindBuffer(resources.get<GpuConstantResource>(ResourceName(StarsParams)),
                              compiledGpi.getConstantBindPoint("StarsParams"));

    context.device.bindTexture(resources.get<GpuTextureResource>(ResourceName(Stars)),
                               compiledGpi.getTextureBindPoint("Stars"));

    if (_atmosphereRenderState)
    {
        const Atmosphere::Model& model = context.scene.sky()->atmosphere()->model();
        model.SetProgramUniforms(compiledGpi.getTextureBindPoint("transmittance_texture").bindPoint,
                                 compiledGpi.getTextureBindPoint("scattering_texture").bindPoint,
                                 GpuProgramTextureBindPoint::invalid().bindPoint,// *compiledGpi.getTextureBindPoint("irradiance_texture").bindPoint*/,
                                 compiledGpi.getTextureBindPoint("single_mie_scattering_texture").bindPoint);

        context.device.bindBuffer(resources.get<GpuConstantResource>(ResourceName(AtmosphereParams)),
                                  compiledGpi.getConstantBindPoint("AtmosphereParams"));

        context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(MoonLighting)),
                                   compiledGpi.getTextureBindPoint("Moon"));
    }
}

void SkyTask::update(GraphicContext& context)
{
    Profile(Sky);

    GpuStarsParams starsParams;
    _hash = toGpu(context, starsParams);

    GpuResourceManager& resources = context.resources;
    resources.update<GpuConstantResource>(ResourceName(StarsParams), {sizeof(starsParams), &starsParams});

    if (_atmosphereRenderState)
    {
        glm::vec4 starsQuaternion;
        glm::vec3 sunDirection, moonDirection, moonUp;
        context.scene.sky()->localization().computeSunAndMoon(sunDirection, moonDirection, _atmosphereRenderState->_moonQuaternion, starsQuaternion);

        const Atmosphere& atmosphere = *context.scene.sky()->atmosphere();
        DirectionalLight& sun = *atmosphere.sun();
        DirectionalLight& moon = *atmosphere.moon();

        sun.setDirection(sunDirection);
        moon.setDirection(moonDirection);
        context.scene.sky()->stars()->setQuaternion(starsQuaternion);

        // Moon transform
        _atmosphereRenderState->_moonTransform = quatMat4(quatConjugate(_atmosphereRenderState->_moonQuaternion));

        // Moon luminance approximation (hexa-web sampling)
        float maxMoonAlbedo = 0.14;
        float L_i = sun.emissionLuminance() * sun.solidAngle() / glm::pi<float>();

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
                glm::vec3 dir_earth = glm::vec3(_atmosphereRenderState->_moonTransform * glm::vec4(dir_orth, 0));

                float backScattering = 2 + glm::dot(-moonDirection, sunDirection) / 3.0f;
                float L_o = maxMoonAlbedo * L_i * glm::max(0.0f, glm::dot(dir_earth, sunDirection)) * backScattering;
                luminanceAvg += glm::vec2(L_o, 1.0f);
            }
        }

        float moonLuminance = luminanceAvg.x / glm::max(1e-5f, luminanceAvg.y);
        moon.setEmissionLuminance(moonLuminance);

        GpuMoonLightParams moonLightParams;
        GpuAtmosphereParams atmosphereParams;
        uint64_t atmosphereHash = toGpu(context, moonLightParams, atmosphereParams);

        _atmosphereRenderState->_moonIsDirty = atmosphereHash != _atmosphereRenderState->_atmosphereHash;
        _atmosphereRenderState->_atmosphereHash = atmosphereHash;
        _hash = combineHashes(atmosphereHash, _hash);

        GpuResourceManager& resources = context.resources;
        resources.update<GpuConstantResource>(ResourceName(AtmosphereParams), {sizeof(atmosphereParams), &atmosphereParams});
        resources.update<GpuConstantResource>(ResourceName(MoonLightParams), {sizeof(moonLightParams), &moonLightParams});
    }
}

void SkyTask::render(GraphicContext& context)
{
    if (_atmosphereRenderState)
    {
        ProfileGpu(Sky);

        if(!_atmosphereRenderState->_moonLightProgram->isValid())
            return;

        if(!_atmosphereRenderState->_moonIsDirty)
            return;

        CompiledGpuProgramInterface compiledGpi;
        if (!_atmosphereRenderState->_moonLightGpi->compile(compiledGpi, *_atmosphereRenderState->_moonLightProgram))
            return;

        GpuMoonLightParams moonParams;
        GpuAtmosphereParams skyParams;
        toGpu(context, moonParams, skyParams);

        GraphicProgramScope programScope(*_atmosphereRenderState->_moonLightProgram);

        GpuResourceManager& resources = context.resources;

        context.device.bindBuffer(resources.get<GpuConstantResource>(ResourceName(MoonLightParams)),
                                  compiledGpi.getConstantBindPoint("MoonLightParams"));
        context.device.bindTexture(resources.get<GpuTextureResource>(ResourceName(MoonAlbedo)),
                                   compiledGpi.getTextureBindPoint("MoonAlbedo"));
        context.device.bindImage(resources.get<GpuImageResource>(ResourceName(MoonLighting)),
                                 compiledGpi.getImageBindPoint("MoonLighting"));

        context.device.dispatch(_atmosphereRenderState->_moonTexSize / 8,
                                _atmosphereRenderState->_moonTexSize / 8);

        _atmosphereRenderState->_moonIsDirty = false;
    }
}

uint64_t SkyTask::toGpu(
    const GraphicContext& context,
    GpuStarsParams& gpuParams) const
{
    const Sky& sky = *context.scene.sky();

    gpuParams.starsQuaternion = sky.stars()->starsQuaternion();
    gpuParams.starsExposure = sky.stars()->starsExposure();

    uint64_t hash = 0;
    hash = hashVal(gpuParams, hash);

    return hash;
}

uint64_t SkyTask::toGpu(
    const GraphicContext& context,
    GpuMoonLightParams& moonLightParams,
    GpuAtmosphereParams& atmosphereParams) const
{
    const Sky& sky = *context.scene.sky();
    const Atmosphere& atmosphere = *sky.atmosphere();

    moonLightParams.transform = _atmosphereRenderState->_moonTransform;
    moonLightParams.sunDirection = glm::vec4(atmosphere.sun()->direction(), 0);
    moonLightParams.moonDirection = glm::vec4(atmosphere.moon()->direction(), 0);
    moonLightParams.sunLi = glm::vec4(atmosphere.sun()->emissionColor() * atmosphere.sun()->emissionLuminance() * atmosphere.sun()->solidAngle() / glm::pi<float>(), 0);

    atmosphereParams.sunDirection = glm::vec4(atmosphere.sun()->direction(), 0);
    atmosphereParams.moonDirection = glm::vec4(atmosphere.moon()->direction(), 0);
    atmosphereParams.moonQuaternion = _atmosphereRenderState->_moonQuaternion;
    atmosphereParams.sunToMoonRatio = atmosphere.moon()->emissionLuminance() / atmosphere.sun()->emissionLuminance();
    atmosphereParams.groundHeightKM = atmosphere.params().bottom_radius.to(atmosphere::reference::km);

    uint64_t hash = 0;
    hash = hashVal(moonLightParams, hash);
    hash = hashVal(atmosphereParams, hash);

    return hash;
}

}
