#include "tracer.h"

#include "random.h"
#include "camera.h"
#include "profiler.h"
#include "material.h"


namespace unisim
{

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

    GPUBindlessTexture blueNoise[PathTracer::BLUE_NOISE_TEX_COUNT];
    glm::vec4 halton[PathTracer::HALTON_SAMPLE_COUNT];
};


PathTracer::PathTracer() :
    GraphicTask("Path Tracer"),
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

void PathTracer::registerDynamicResources(GraphicContext& context)
{
    ResourceManager& resources = context.resources;

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
    {
        _blueNoiseTextureResourceIds[i] = resources.registerResource("Blue Noise Texture " + std::to_string(i));
        _blueNoiseBindlessResourceIds[i] = resources.registerResource("Blue Noise Bindless " + std::to_string(i));
    }
}

bool PathTracer::definePathTracerModules(GraphicContext& context)
{
    if(!addPathTracerModule(_computePathTraceShaderId, context.settings, "shaders/pathtrace.glsl"))
        return false;

    if(!addPathTracerModule(_pathTraceUtilsShaderId, context.settings, "shaders/common/utils.glsl"))
        return false;

    return true;
}

bool PathTracer::defineResources(GraphicContext& context)
{
    bool ok = true;

    ResourceManager& resources = context.resources;

    std::vector<GLuint> pathTracerModules = context.resources.pathTracerModules();

    // Generate programs
    if(!generateComputeProgram(_computePathTraceProgramId, "pathtracer", pathTracerModules))
        return false;

    _pathTracerInterface.reset(new PathTracerInterface(_computePathTraceProgramId));

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
    {
        std::string blueNoiseName = "LDR_RGBA_" + std::to_string(i) + ".png";

        if(Texture* texture = Texture::load("textures/bluenoise64/" + blueNoiseName))
        {
            ok = ok && resources.define<GpuTextureResource>(_blueNoiseTextureResourceIds[i], {*texture});
            GLuint texId = resources.get<GpuTextureResource>(_blueNoiseTextureResourceIds[i]).texId;
            ok = ok && resources.define<GpuBindlessResource>(_blueNoiseBindlessResourceIds[i], {texture, texId});
        }
        else
        {
            std::cerr << "Cannot load blue noise texture. Did you install the texture pack?" << std::endl;
            _blueNoiseTextureResourceIds[i] = Invalid_ResourceId;
            _blueNoiseBindlessResourceIds[i] = Invalid_ResourceId;
        }
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

    _pathTraceUnit = 0;
    _pathTraceLoc = glGetUniformLocation(_computePathTraceProgramId, "result");
    glProgramUniform1i(_computePathTraceProgramId, _pathTraceLoc, _pathTraceUnit);

    _pathTracerHash = context.resources.pathTracerHash();

    return ok;
}

void PathTracer::setPathTracerResources(
        GraphicContext &context,
        PathTracerInterface &interface) const
{
    ResourceManager& resources = context.resources;

    glBindBufferBase(GL_UNIFORM_BUFFER, interface.getUboBindPoint("PathTracerCommonParams"),
                     resources.get<GpuConstantResource>(ResourceName(PathTracerCommonParams)).bufferId);
}

void PathTracer::update(GraphicContext& context)
{
    const Camera& camera = context.camera;
    const Viewport& viewport = camera.viewport();
    ResourceManager& resources = context.resources;

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

    ResourceManager& resources = context.resources;

    GLuint pathTraceTexId = resources.get<GpuImageResource>(ResourceName(PathTracerResult)).texId;

    if(_frameIndex < MAX_FRAME_COUNT)
    {
        glUseProgram(_computePathTraceProgramId);

        // Set uniforms for sub-systems
        resources.setPathTracerResources(context, *_pathTracerInterface);

        glBindImageTexture(_pathTraceUnit, pathTraceTexId, 0, false, 0, GL_WRITE_ONLY, _pathTraceFormat);

        glDispatchCompute((_viewport->width + 7) / 8, (_viewport->height + 3) / 4, 1);
    }

    glUseProgram(0);
}

uint64_t PathTracer::toGpu(
        GraphicContext& context,
        GpuPathTracerCommonParams& gpuCommonParams)
{
    ResourceManager& resources = context.resources;

    const Camera& camera = context.camera;

    glm::mat4 view(glm::mat3(camera.view()));
    glm::mat4 proj = camera.proj();
    glm::mat4 screen = camera.screen();
    glm::mat4 viewToScreen = screen * proj * view;

    gpuCommonParams.rayMatrix = glm::inverse(viewToScreen);
    gpuCommonParams.lensePosition = glm::vec4(camera.position(), 1);
    gpuCommonParams.lenseDirection = glm::vec4(camera.direction(), 0);
    gpuCommonParams.focusDistance = camera.focusDistance();
    gpuCommonParams.apertureRadius = camera.dofEnable() ? camera.focalLength() / camera.fstop() * 0.5f : 0.0f;
    gpuCommonParams.exposure = camera.exposure();
    gpuCommonParams.frameIndex = 0; // Must be constant for hasing

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
        gpuCommonParams.blueNoise[i].texture = resources.get<GpuBindlessResource>(_blueNoiseBindlessResourceIds[i]).handle;
    for(unsigned int i = 0; i < HALTON_SAMPLE_COUNT; ++i)
        gpuCommonParams.halton[i] = _halton[i];

    uint64_t hash = 0;
    hash = hashVal(gpuCommonParams, hash);

    return hash;
}

}
