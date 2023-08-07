#include "radiation.h"

#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <functional>
#include <string_view>

#include "GLM/gtc/constants.hpp"

#include "scene.h"
#include "object.h"
#include "body.h"
#include "primitive.h"
#include "camera.h"
#include "units.h"
#include "material.h"
#include "random.h"
#include "profiler.h"
#include "sky.h"
#include "terrain.h"


namespace unisim
{

DeclareProfilePointGpu(Radiation);
DeclareProfilePointGpu(PathTrace);

DefineResource(PathTrace);
DeclareResource(MoonLighting);

DefineResource(PathTracerCommonParams);
DefineResource(DirectionalLights);
DefineResource(Emitters);

struct GpuEmitter
{
    GLuint instance;
    GLuint primitive;
};

struct GpuDirectionalLight
{
    glm::vec4 directionCosThetaMax;
    glm::vec4 emissionSolidAngle;
};

struct GpuPathTracerCommonParams
{
    glm::mat4 rayMatrix;
    glm::vec4 lensePosition;
    glm::vec4 lenseDirection;
    GLfloat focusDistance;
    GLfloat apertureRadius;
    GLfloat exposure;

    GLuint frameIndex;

    GPUBindlessTexture blueNoise[Radiation::BLUE_NOISE_TEX_COUNT];
    glm::vec4 halton[Radiation::HALTON_SAMPLE_COUNT];
};

Radiation::Radiation() :
    GraphicTask("Radiation"),
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

glm::vec3 toLinear(const glm::vec3& sRGB)
{
    glm::bvec3 cutoff = glm::lessThan(sRGB, glm::vec3(0.04045));
    glm::vec3 higher = glm::pow((sRGB + glm::vec3(0.055))/glm::vec3(1.055), glm::vec3(2.4));
    glm::vec3 lower = sRGB/glm::vec3(12.92);

    return glm::mix(higher, lower, cutoff);
}

void Radiation::registerDynamicResources(GraphicContext& context)
{
    ResourceManager& resources = context.resources;

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
    {
        _blueNoiseTextureResourceIds[i] = resources.registerResource("Blue Noise Texture " + std::to_string(i));
        _blueNoiseBindlessResourceIds[i] = resources.registerResource("Blue Noise Bindless " + std::to_string(i));
    }
}

bool Radiation::definePathTracerModules(GraphicContext& context)
{
    if(!addPathTracerModule(_computePathTraceShaderId, context.settings, "shaders/pathtrace.glsl"))
        return false;

    if(!addPathTracerModule(_pathTraceUtilsShaderId, context.settings, "shaders/common/utils.glsl"))
        return false;

    return true;
}

bool Radiation::defineResources(GraphicContext& context)
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
        std::stringstream ss;
        ss << "LDR_RGBA_" << i << ".png";
        std::string blueNoiseName = ss.str();

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
    std::vector<GpuEmitter> gpuEmitters;
    std::vector<GpuDirectionalLight> gpuDirectionalLights;

    _hash = toGpu(
        context,
        gpuCommonParams,
        gpuEmitters,
        gpuDirectionalLights);

    ok = ok && resources.define<GpuConstantResource>(
                ResourceName(PathTracerCommonParams), {
                    sizeof(GpuPathTracerCommonParams),
                    &gpuCommonParams});

    ok = ok && resources.define<GpuStorageResource>(
                ResourceName(Emitters), {
                    sizeof(gpuEmitters),
                    gpuEmitters.size(),
                    gpuEmitters.data()});

    ok = ok && resources.define<GpuStorageResource>(
                ResourceName(DirectionalLights), {
                    sizeof(GpuDirectionalLight),
                    gpuDirectionalLights.size(),
                    gpuDirectionalLights.data()});

    _viewport = context.camera.viewport();

    _pathTraceFormat = GL_RGBA32F;
    ok = ok && resources.define<GpuImageResource>(
                ResourceName(PathTrace), {
                    _viewport.width,
                    _viewport.height,
                    _pathTraceFormat});

    _pathTraceUnit = 0;
    _pathTraceLoc = glGetUniformLocation(_computePathTraceProgramId, "result");
    glProgramUniform1i(_computePathTraceProgramId, _pathTraceLoc, _pathTraceUnit);

    _pathTracerHash = context.resources.pathTracerHash();

    return ok;
}

void Radiation::setPathTracerResources(
        GraphicContext &context,
        PathTracerInterface &interface) const
{
    ResourceManager& resources = context.resources;

    glBindBufferBase(GL_UNIFORM_BUFFER, interface.getUboBindPoint("PathTracerCommonParams"),
                     resources.get<GpuConstantResource>(ResourceName(PathTracerCommonParams)).bufferId);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("Emitters"),
                     resources.get<GpuStorageResource>(ResourceName(Emitters)).bufferId);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("DirectionalLights"),
                     resources.get<GpuStorageResource>(ResourceName(DirectionalLights)).bufferId);

}

void Radiation::update(GraphicContext& context)
{
    const Camera& camera = context.camera;
    const Viewport& viewport = camera.viewport();
    ResourceManager& resources = context.resources;

    if(_viewport != viewport)
    {
        _viewport = viewport;
        resources.define<GpuImageResource>(
                    ResourceName(PathTrace), {
                        viewport.width,
                        viewport.height,
                        _pathTraceFormat});
    }

    GpuPathTracerCommonParams gpuCommonParams;
    std::vector<GpuEmitter> gpuEmitters;
    std::vector<GpuDirectionalLight> gpuDirectionalLights;

    uint64_t hash = toGpu(
        context,
        gpuCommonParams,
        gpuEmitters,
        gpuDirectionalLights);

    if(_hash != hash)
    {
        resources.get<GpuStorageResource>(
                    ResourceName(Emitters)).update({
                        sizeof(gpuEmitters),
                        gpuEmitters.size(),
                        gpuEmitters.data()});

        resources.get<GpuStorageResource>(
                    ResourceName(DirectionalLights)).update({
                        sizeof(GpuDirectionalLight),
                        gpuDirectionalLights.size(),
                        gpuDirectionalLights.data()});

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

void Radiation::render(GraphicContext& context)
{
    ProfileGpu(Radiation);

    ResourceManager& resources = context.resources;

    GLuint pathTraceTexId = resources.get<GpuImageResource>(ResourceName(PathTrace)).texId;

    if(_frameIndex < MAX_FRAME_COUNT)
    {
        ProfileGpu(PathTrace);

        glUseProgram(_computePathTraceProgramId);

        // Set uniforms for sub-systems
        resources.setPathTracerResources(context, *_pathTracerInterface);

        glBindImageTexture(_pathTraceUnit, pathTraceTexId, 0, false, 0, GL_WRITE_ONLY, _pathTraceFormat);

        glDispatchCompute((_viewport.width + 7) / 8, (_viewport.height + 3) / 4, 1);
    }

    glUseProgram(0);
}

uint64_t Radiation::toGpu(
        GraphicContext& context,
        GpuPathTracerCommonParams& gpuCommonParams,
        std::vector<GpuEmitter>& gpuEmitters,
        std::vector<GpuDirectionalLight>& gpuDirectionalLights)
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

    const auto& objects = context.scene.objects();
    for(std::size_t i = 0; i < objects.size(); ++i)
    {
        const Object& object = *objects[i];
        for(std::size_t p = 0; p < object.primitives().size(); ++p)
        {
            const Primitive& primitive = *object.primitives()[p];
            if(glm::any(glm::greaterThan(primitive.material()->defaultEmissionColor(), glm::vec3())))
            {
                GpuEmitter& gpuEmitter = gpuEmitters.emplace_back();
                gpuEmitter.instance = i;
                gpuEmitter.primitive = p;
            }
        }
    }

    const auto& directionalLights = context.scene.sky()->directionalLights();
    gpuDirectionalLights.reserve(directionalLights.size());
    for(std::size_t i = 0; i < directionalLights.size(); ++i)
    {
        const std::shared_ptr<DirectionalLight>& directionalLight = directionalLights[i];
        GpuDirectionalLight& gpuDirectionalLight = gpuDirectionalLights.emplace_back();
        gpuDirectionalLight.directionCosThetaMax = glm::vec4(
                    directionalLight->direction(),
                    1 - directionalLight->solidAngle() / (2 * glm::pi<float>()));
        gpuDirectionalLight.emissionSolidAngle = glm::vec4(
                    directionalLight->emissionColor() * directionalLight->emissionLuminance(),
                    directionalLight->solidAngle());
    }

    uint64_t hash = 0;
    hash = hashVal(gpuCommonParams, hash);
    hash = hashVec(gpuEmitters, hash);
    hash = hashVec(gpuDirectionalLights, hash);

    return hash;
}

}
