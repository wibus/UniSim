#include "tracer.h"

#include <iostream>
#include <random>

#include "../system/profiler.h"
#include "../system/random.h"

#include "../resource/texture.h"

#include "../graphic/graphic.h"

#include "camera.h"


namespace unisim
{

DefineProfilePoint(PathTracer);
DefineProfilePointGpu(PathTracer);

DefineResource(PathTracerResult);
DefineResource(PathTracerCommonParams);


struct GpuPathTracerCommonParams
{
    glm::mat4 rayMatrix;
    glm::vec4 lensePosition;
    glm::vec4 lenseDirection;
    GLfloat focusDistance;
    GLfloat apertureRadius;
    GLfloat exposure;

    GLuint frameIndex;
    
    GpuBindlessTextureDescriptor blueNoise[PathTracer::BLUE_NOISE_TEX_COUNT];
    glm::vec4 halton[PathTracer::HALTON_SAMPLE_COUNT];
};


PathTracer::PathTracer() :
    GraphicTask("Path Tracer"),
    _frameIndex(0),
    _pathTracerHash(0)
{
    _utilsModule = registerPathTracerModule("Utils");
    _pathTraceModule = registerPathTracerModule("Path Trace");
    _pathTracerProgram = registerProgram("Path Tracer");

    for(unsigned int i = 0; i < HALTON_SAMPLE_COUNT; ++i)
    {
        double* sample = halton(i, 4);
        _halton[i] = glm::vec4(sample[0], sample[1], sample[2], sample[4]);
        delete sample;
    }
}

void PathTracer::registerDynamicResources(GraphicContext& context)
{
    GpuResourceManager& resources = context.resources;

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
    {
        _blueNoiseTextureResourceIds[i] = resources.registerDynamicResource("Blue Noise Texture " + std::to_string(i));
        _blueNoiseBindlessResourceIds[i] = resources.registerDynamicResource("Blue Noise Bindless " + std::to_string(i));
    }
}

bool PathTracer::definePathTracerModules(GraphicContext& context)
{
    if(!addPathTracerModule(*_pathTraceModule, context.settings, "shaders/pathtrace.glsl"))
        return false;

    if(!addPathTracerModule(*_utilsModule, context.settings, "shaders/common/utils.glsl"))
        return false;

    return true;
}

bool PathTracer::definePathTracerInterface(GraphicContext& context, PathTracerInterface& interface)
{
    bool ok = true;

    ok = ok && interface.declareConstant("PathTracerCommonParams");

    return ok;
}

bool PathTracer::defineShaders(GraphicContext &context)
{
    // Generate programs
    std::vector<std::shared_ptr<GraphicShader>> shaders;
    for (const auto& module : context.resources.pathTracerModules())
    {
        if (module)
        {
            shaders.push_back(module->shader());
        }
    }

    if(!generateComputeProgram(*_pathTracerProgram, "pathtracer", shaders))
        return false;

    return _pathTracerProgram->isValid();
}

bool PathTracer::defineResources(GraphicContext& context)
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
            texture->format = TextureFormat::UNORM8;
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
        ok = ok && resources.define<GpuBindlessResource>(_blueNoiseBindlessResourceIds[i], {texture, textureResource});

        delete texture;
    }

    GpuPathTracerCommonParams gpuCommonParams;

    _hash = toGpu(
        context,
        gpuCommonParams);

    ok = ok && resources.define<GpuConstantResource>(
                ResourceName(PathTracerCommonParams), {
                    sizeof(GpuPathTracerCommonParams),
                    &gpuCommonParams});

    _viewport.reset(new Viewport(context.camera.viewport()));

    _pathTraceFormat = GL_RGBA32F;
    ok = ok && resources.define<GpuImageResource>(
                ResourceName(PathTracerResult), {
                    _viewport->width,
                    _viewport->height,
                    _pathTraceFormat});

    _pathTraceUnit = GpuProgramImageUnit::first();
    _pathTraceLoc = glGetUniformLocation(_pathTracerProgram->handle(), "result");
    glProgramUniform1i(_pathTracerProgram->handle(), _pathTraceLoc, _pathTraceUnit.unit);

    _pathTracerHash = context.resources.pathTracerHash();

    return ok;
}

void PathTracer::setPathTracerResources(
    GraphicContext &context,
        PathTracerInterface &interface) const
{
    GpuResourceManager& resources = context.resources;

    context.device.bindBuffer(resources.get<GpuConstantResource>(ResourceName(PathTracerCommonParams)),
                              interface.getConstantBindPoint("PathTracerCommonParams"));
}

void PathTracer::update(GraphicContext& context)
{
    Profile(PathTracer);

    const Camera& camera = context.camera;
    const Viewport& viewport = camera.viewport();
    GpuResourceManager& resources = context.resources;

    if(*_viewport != viewport)
    {
        *_viewport = viewport;
        resources.define<GpuImageResource>(
                    ResourceName(PathTracerResult), {
                        viewport.width,
                        viewport.height,
                        _pathTraceFormat});
    }

    GpuPathTracerCommonParams gpuCommonParams;

    uint64_t hash = toGpu(
        context,
        gpuCommonParams);

    if(_hash != hash)
    {
        _hash = hash;
    }

    u_int64_t pathTracerHash = context.resources.pathTracerHash();
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

    resources.get<GpuConstantResource>(
                ResourceName(PathTracerCommonParams)).update({
                    sizeof(GpuPathTracerCommonParams),
                    &gpuCommonParams});
}

void PathTracer::render(GraphicContext& context)
{
    ProfileGpu(PathTracer);

    if(!_pathTracerProgram->isValid())
        return;

    if(!_pathTracerInterface->isValid())
        return;
    
    GpuResourceManager& resources = context.resources;

    const auto& pathTracerResult = resources.get<GpuImageResource>(ResourceName(PathTracerResult));

    if(_frameIndex < MAX_FRAME_COUNT)
    {
        GraphicProgramScope programScope(*_pathTracerProgram);

        // Set uniforms for sub-systems
        resources.setPathTracerResources(context, *_pathTracerInterface);

        context.device.bindImage(pathTracerResult, _pathTraceUnit);

        context.device.dispatch((_viewport->width + 7) / 8, (_viewport->height + 3) / 4);
    }
}

std::shared_ptr<PathTracerInterface> PathTracer::initInterface()
{
    _pathTracerInterface.reset(new PathTracerInterface(_pathTracerProgram));
    return _pathTracerInterface;
}

uint64_t PathTracer::toGpu(
    GraphicContext& context,
    GpuPathTracerCommonParams& gpuCommonParams)
{
    GpuResourceManager& resources = context.resources;

    const Camera& camera = context.camera;

    glm::mat4 view(glm::mat3(camera.view()));
    glm::mat4 proj = camera.proj();
    glm::mat4 screen = camera.screen();
    glm::mat4 viewToScreen = screen * proj * view;

    gpuCommonParams.rayMatrix = glm::inverse(viewToScreen);
    gpuCommonParams.lensePosition = glm::vec4(camera.position(), 1);
    gpuCommonParams.lenseDirection = glm::vec4(camera.direction(), 0);
    gpuCommonParams.focusDistance = camera.focusDistance();
    gpuCommonParams.apertureRadius = camera.dofEnabled() ? camera.focalLength() / camera.fstop() * 0.5f : 0.0f;
    gpuCommonParams.exposure = camera.exposure();
    gpuCommonParams.frameIndex = 0; // Must be constant for hasing

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
        gpuCommonParams.blueNoise[i] = resources.get<GpuBindlessResource>(_blueNoiseBindlessResourceIds[i]).handle();
    for(unsigned int i = 0; i < HALTON_SAMPLE_COUNT; ++i)
        gpuCommonParams.halton[i] = _halton[i];

    uint64_t hash = 0;
    hash = hashVal(gpuCommonParams, hash);

    return hash;
}

}
