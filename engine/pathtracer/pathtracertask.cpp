#include "pathtracertask.h"

#include <algorithm>
#include <iostream>
#include <random>

#include "../system/profiler.h"
#include "../system/random.h"

#include "../resource/texture.h"

#include "../graphic/graphic.h"
#include "../graphic/gpudevice.h"

#include "../camera.h"


namespace unisim
{

DefineProfilePoint(PathTracer);
DefineProfilePointGpu(PathTracer);

DefineResource(PathTracerResult);
DefineResource(PathTracerCommonParams);

DefineResource(BindlessTextures);


struct GpuPathTracerCommonParams
{
    glm::mat4 rayMatrix;
    glm::vec4 lensePosition;
    glm::vec4 lenseDirection;
    glm::float32 focusDistance;
    glm::float32 apertureRadius;
    glm::float32 exposure;

    glm::uint32 frameIndex;

    glm::ivec4 blueNoiseTextures[PathTracerTask::BLUE_NOISE_TEX_COUNT];
    glm::vec4 halton[PathTracerTask::HALTON_SAMPLE_COUNT];
};


PathTracerTask::PathTracerTask() :
    PathTracerProviderTask("Path Tracer"),
    _frameIndex(0),
    _pathTracerHash(0)
{
    for(unsigned int i = 0; i < HALTON_SAMPLE_COUNT; ++i)
    {
        double* sample = halton(i, 4);
        _halton[i] = glm::vec4(sample[0], sample[1], sample[2], sample[4]);
        delete sample;
    }
}

void PathTracerTask::registerDynamicResources(GraphicContext& context)
{
    GpuResourceManager& resources = context.resources;

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
    {
        _blueNoiseTextureResourceIds[i] = resources.registerDynamicResource("Blue Noise Texture " + std::to_string(i));
        _blueNoiseBindlessResourceIds[i] = Invalid_BindlessTextureIndex;
    }
}

bool PathTracerTask::defineResources(GraphicContext& context)
{
    bool ok = true;

    GpuResourceManager& resources = context.resources;

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
    {
        std::string blueNoiseName = "LDR_RGBA_" + std::to_string(i) + ".png";

        Texture* texture = nullptr;
        if(!(texture = Texture::load("textures/bluenoise64/" + blueNoiseName)))
        {
            std::cerr << "Cannot load blue noise texture. Did you install the texture pack?" << std::endl;

            texture = new Texture();
            texture->width = 64;
            texture->height = 64;
            texture->format = TextureFormat::R8G8B8A8_UNORM;
            texture->numComponents = 4;
            texture->data.resize(64*64*4);

            std::seed_seq seed{i};
            std::mt19937 eng(seed);
            std::uniform_real_distribution<double> urd(0, 1);

            for(unsigned int j = 0; j < 64*64; ++j)
            {
                texture->data[j*4] = glm::clamp<unsigned char>(urd(eng) * 255, 0, 255);
                texture->data[j*4+1] = glm::clamp<unsigned char>(urd(eng) * 255, 0, 255);
                texture->data[j*4+2] = glm::clamp<unsigned char>(urd(eng) * 255, 0, 255);
                texture->data[j*4+3] = glm::clamp<unsigned char>(urd(eng) * 255, 0, 255);
            }
        }

        ok = ok && resources.define<GpuTextureResource>(_blueNoiseTextureResourceIds[i], {*texture});
        const auto& textureResource = resources.get<GpuTextureResource>(_blueNoiseTextureResourceIds[i]);
        _blueNoiseBindlessResourceIds[i] = resources.registerBindlessTexture(textureResource, *texture);
        ok = ok && (_blueNoiseBindlessResourceIds[i] != Invalid_BindlessTextureIndex);

        delete texture;
    }

    GpuPathTracerCommonParams gpuCommonParams;

    _hash = toGpu(
        context,
        gpuCommonParams);

    _viewport.reset(new Viewport(context.camera.viewport()));

    ok = ok && resources.define<GpuConstantResource>(
            ResourceName(PathTracerCommonParams), {
                sizeof(GpuPathTracerCommonParams),
                &gpuCommonParams});

    ok = ok && resources.define<GpuImageResource>(
            ResourceName(PathTracerResult), {
                .width  = _viewport->width,
                .height = _viewport->height,
                .depth  = 1,
                .format = TextureFormat::R32G32B32A32_FLOAT});

    return ok;
}

bool PathTracerTask::defineShaders(GraphicContext& context)
{
    _pathTracerModules.clear();
    GpuProgramInterfaceTemplate pathTraceGpiTemplate(context.resources.frameGpiTemplate());

    for (auto& task : _pathTracerProviders)
    {
        if (!task->definePathTracerModules(context, _pathTracerModules))
        {
            PILS_ERROR("Could not define path tracer module for: ", task->name());
            return false;
        }

        if (!task->definePathTracerInterface(context, pathTraceGpiTemplate))
        {
            PILS_ERROR("Could not define path tracer interface for: ", task->name());
            return false;
        }
    }

    _pathTracerInterface.reset(new GpuProgramInterface(context.device, pathTraceGpiTemplate));

    // Remove duplicated modules
    std::sort( _pathTracerModules.begin(), _pathTracerModules.end() );
    _pathTracerModules.erase( std::unique( _pathTracerModules.begin(), _pathTracerModules.end() ), _pathTracerModules.end());

    // Gather shaders
    std::vector<std::shared_ptr<GraphicShader>> shaders;
    for (const auto& module : _pathTracerModules)
        if (module)
            shaders.push_back(module->shader());

    // Generate program
    _pathTracerProgram.reset();
    if(!generateComputeProgram(context.device, pathTraceGpiTemplate, _pathTracerProgram, "Path Tracer", {shaders}))
        return false;

    return true;
}

bool PathTracerTask::definePathTracerModules(GraphicContext& context, std::vector<std::shared_ptr<PathTracerModule>>& modules)
{
    if(!addPathTracerModule(modules, "Path Trace", context.settings, "shaders/pathtrace.glsl"))
        return false;

    if(!addPathTracerModule(modules, "Utils", context.settings, "shaders/common/utils.glsl"))
        return false;

    return true;
}

bool PathTracerTask::definePathTracerInterface(GraphicContext& context, GpuProgramInterfaceTemplate& gpiTemplate)
{
    bool ok = true;

    gpiTemplate.declareConstant({"PathTracerCommonParams"});
    gpiTemplate.declareImage({"result"});

    return ok;
}

void PathTracerTask::bindPathTracerResources(
    GraphicContext &context,
    GpuProgramInterface& gpi) const
{
    GpuResourceManager& resources = context.resources;
    gpi.setConstant("PathTracerCommonParams", resources.get<GpuConstantResource>(ResourceName(PathTracerCommonParams)));
    gpi.setImage("result", resources.get<GpuImageResource>(ResourceName(PathTracerResult)));
}

void PathTracerTask::update(GraphicContext& context)
{
    Profile(PathTracer);

    const Camera& camera = context.camera;
    const Viewport& viewport = camera.viewport();
    GpuResourceManager& resources = context.resources;

    if(*_viewport != viewport)
    {
        *_viewport = viewport;
        resources.update<GpuImageResource>(
            ResourceName(PathTracerResult), {
                .width  = viewport.width,
                .height = viewport.height,
                .depth  = 1,
                .format = TextureFormat::R32G32B32A32_FLOAT});
    }

    GpuPathTracerCommonParams gpuCommonParams;

    // Provier hash
    uint64_t hash = toGpu(
        context,
        gpuCommonParams);

    if(_hash != hash)
    {
        _hash = hash;
    }

    // Path Tracer hash
    u_int64_t pathTracerHash = 0;
    for(const auto& provider : _pathTracerProviders)
        pathTracerHash = PathTracerProviderTask::combineHashes(pathTracerHash, provider->hash());

    if(_pathTracerHash != pathTracerHash)
    {
        _frameIndex = 0;
        _pathTracerHash = pathTracerHash;
    }
    else
    {
        ++_frameIndex;
    }

    gpuCommonParams.frameIndex = _frameIndex;

    resources.update<GpuConstantResource>(
        ResourceName(PathTracerCommonParams), {
            sizeof(GpuPathTracerCommonParams),
            &gpuCommonParams});
}

void PathTracerTask::render(GraphicContext& context)
{
    ProfileGpu(PathTracer);

    if(_frameIndex < MAX_FRAME_COUNT)
    {
        if(!_pathTracerProgram->isValid())
            return;

        if(!context.resources.compileAndSetPassGpi(*_pathTracerProgram))
            return;

        for(const auto& provider : _pathTracerProviders)
            provider->bindPathTracerResources(context, *_pathTracerInterface);

        if(!_pathTracerInterface->compileAndSet(context.device, *_pathTracerProgram))
            return;

        GraphicProgramScope programScope(*_pathTracerProgram);
        context.device.dispatch((_viewport->width + 7) / 8, (_viewport->height + 3) / 4);
    }
}

void PathTracerTask::setPathTracerProviders(const std::vector<PathTracerProviderTaskPtr>& providers)
{
    _pathTracerProviders = providers;
}

uint64_t PathTracerTask::toGpu(
    GraphicContext& context,
    GpuPathTracerCommonParams& gpuParams)
{
    GpuResourceManager& resources = context.resources;

    const Camera& camera = context.camera;

    glm::mat4 view(glm::mat3(camera.view()));
    glm::mat4 proj = camera.proj();
    glm::mat4 screen = camera.screen();
    glm::mat4 viewToScreen = screen * proj * view;

    gpuParams.rayMatrix = glm::inverse(viewToScreen);
    gpuParams.lensePosition = glm::vec4(camera.position(), 1);
    gpuParams.lenseDirection = glm::vec4(camera.direction(), 0);
    gpuParams.focusDistance = camera.focusDistance();
    gpuParams.apertureRadius = camera.dofEnabled() ? camera.focalLength() / camera.fstop() * 0.5f : 0.0f;
    gpuParams.exposure = camera.exposure();
    gpuParams.frameIndex = 0; // Must be constant for hasing

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
        gpuParams.blueNoiseTextures[i].x = int(_blueNoiseBindlessResourceIds[i]);
    for(unsigned int i = 0; i < HALTON_SAMPLE_COUNT; ++i)
        gpuParams.halton[i] = _halton[i];

    uint64_t hash = 0;
    hash = hashVal(gpuParams, hash);

    return hash;
}

}
