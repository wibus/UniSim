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
#include "mesh.h"
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
DeclareProfilePointGpu(Clear);
DeclareProfilePointGpu(PathTrace);
DeclareProfilePointGpu(ColorGrade);

DefineResource(PathTrace);
DeclareResource(MoonLighting);

struct GpuInstance
{
    glm::vec4 albedo;
    glm::vec4 emission;
    glm::vec4 specular;
    glm::vec4 position;
    glm::vec4 quaternion;

    float radius;
    float mass;

    uint materialId;
    uint primitiveType;
};

struct GpuDirectionalLight
{
    glm::vec4 directionCosThetaMax;
    glm::vec4 emissionSolidAngle;
};

struct GPUBindlessTexture
{
    GPUBindlessTexture() :
        texture(0),
        padding(0)
    {}

    GPUBindlessTexture(GLuint64 tex) :
        texture(tex),
        padding(0)
    {}

    GLuint64 texture;
    GLuint64 padding;
};

struct GpuMaterial
{
    GPUBindlessTexture albedo;
    GPUBindlessTexture specular;
};

struct CommonParams
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

    // Background
    glm::vec4 backgroundQuat;
    float backgroundExposure;
};

Radiation::Radiation() :
    GraphicTask("Radiation"),
    _frameIndex(0),
    _lastFrameHash(0)
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
    const Scene& scene = context.scene;
    ResourceManager& resources = context.resources;

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
    {
        _blueNoiseTextureResourceIds[i] = resources.registerResource("Blue Noise Texture 64");
        _blueNoiseBindlessResourceIds[i] = resources.registerResource("Blue Noise Bindless 64");
    }


    const std::vector<std::shared_ptr<Object>>& objects = scene.objects();
    _objectsResourceIds.resize(objects.size());
    for(std::size_t i = 0; i < objects.size(); ++i)
    {
        _objectsResourceIds[i] = {
            resources.registerResource(objects[i]->name() + "_texture_albedo"),
            resources.registerResource(objects[i]->name() + "_texture_specular"),
            resources.registerResource(objects[i]->name() + "_bindless_albedo"),
            resources.registerResource(objects[i]->name() + "_bindless_specular"),
        };
    }
}

bool Radiation::defineResources(GraphicContext& context)
{
    const Scene& scene = context.scene;
    const Viewport& viewport = context.camera.viewport();
    ResourceManager& resources = context.resources;

    const std::vector<std::shared_ptr<Object>>& objects = scene.objects();

    std::vector<GLuint> pathTracerModules = context.resources.pathTracerModules();

    // Generate programs
    if(!generateComputeProgram(_computePathTraceProgramId, "pathtracer", pathTracerModules))
        return false;
    if(!generateGraphicProgram(_colorGradingId, "shaders/fullscreen.vert", "shaders/colorgrade.frag"))
        return false;

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
    {
        std::stringstream ss;
        ss << "LDR_RGBA_" << i << ".png";
        std::string blueNoiseName = ss.str();

        if(Texture* texture = Texture::load("textures/bluenoise64/" + blueNoiseName))
        {
            resources.define<GpuTextureResource>(_blueNoiseTextureResourceIds[i], {*texture});
            GLuint texId = resources.get<GpuTextureResource>(_blueNoiseTextureResourceIds[i]).texId;
            resources.define<GpuBindlessResource>(_blueNoiseBindlessResourceIds[i], {texture, texId});
        }
        else
        {
            std::cerr << "Cannot load blue noise texture. Did you install the texture pack?" << std::endl;
            _blueNoiseTextureResourceIds[i] = Invalid_ResourceId;
            _blueNoiseBindlessResourceIds[i] = Invalid_ResourceId;
        }
    }

    std::vector<GpuMaterial> gpuMaterials;

    _objectToMat.resize(objects.size());
    for(std::size_t i = 0; i < objects.size(); ++i)
    {
        if(objects[i]->material().get() != nullptr && objects[i]->material()->albedo())
        {
            const Material& material = *objects[i]->material();

            GLuint64 alebdoHdl = 0;

            if(material.albedo() != nullptr)
            {
                resources.define<GpuTextureResource>(_objectsResourceIds[i].textureAlbedo, {*material.albedo()});
                GLuint texId = resources.get<GpuTextureResource>(_objectsResourceIds[i].textureAlbedo).texId;
                resources.define<GpuBindlessResource>(_objectsResourceIds[i].bindlessAlbedo, {material.albedo(), texId});
                alebdoHdl = resources.get<GpuBindlessResource>(_objectsResourceIds[i].bindlessAlbedo).handle;
            }

            GpuMaterial gpuMat = {GPUBindlessTexture(alebdoHdl), GPUBindlessTexture(0)};

            gpuMaterials.push_back(gpuMat);
            _objectToMat[i] = gpuMaterials.size();
        }
        else
        {
            _objectToMat[i] = 0;
        }
    }

    float points[] = {
      -1.0f, -1.0f,  0.0f,
       3.0f, -1.0f,  0.0f,
      -1.0f,  3.0f,  0.0f,
    };

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), points, GL_STATIC_DRAW);

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glGenBuffers(1, &_commonUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, _commonUbo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CommonParams), nullptr, GL_STREAM_DRAW);

    glGenBuffers(1, &_instancesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _instancesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_STREAM_DRAW);

    glGenBuffers(1, &_dirLightsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _dirLightsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_STREAM_DRAW);

    glGenBuffers(1, &_materialsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _materialsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuMaterials.size() * sizeof(GpuMaterial), gpuMaterials.data(), GL_STREAM_DRAW);

    glGenBuffers(1, &_emittersSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _emittersSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_STREAM_DRAW);

    _pathTraceFormat = GL_RGBA32F;
    resources.define<GpuImageResource>(ResourceName(PathTrace), {viewport.width, viewport.height, _pathTraceFormat});
    _viewport = viewport;

    _pathTraceUnit = 0;
    _pathTraceLoc = glGetUniformLocation(_computePathTraceProgramId, "result");
    glProgramUniform1i(_computePathTraceProgramId, _pathTraceLoc, _pathTraceUnit);

    return true;
}

bool Radiation::definePathTracerModules(GraphicContext& context)
{
    if(!addPathTracerModule(_computePathTraceShaderId, context.settings, "shaders/pathtrace.glsl"))
        return false;

    if(!addPathTracerModule(_pathTraceUtilsShaderId, context.settings, "shaders/common/utils.glsl"))
        return false;

    return true;
}

void Radiation::update(GraphicContext& context)
{

}

void Radiation::render(GraphicContext& context)
{
    const Scene& scene = context.scene;
    const Camera& camera = context.camera;
    const Viewport& viewport = camera.viewport();
    ResourceManager& resources = context.resources;

    if(_viewport != viewport)
    {
        _viewport = viewport;
        resources.define<GpuImageResource>(ResourceName(PathTrace), {viewport.width, viewport.height, _pathTraceFormat});
    }

    ProfileGpu(Radiation);

    glm::mat4 view(glm::mat3(camera.view()));
    glm::mat4 proj = camera.proj();
    glm::mat4 screen = camera.screen();
    glm::mat4 viewToScreen = screen * proj * view;

    CommonParams params;
    params.rayMatrix = glm::inverse(viewToScreen);
    params.lensePosition = glm::vec4(camera.position(), 1);
    params.lenseDirection = glm::vec4(camera.direction(), 0);
    params.focusDistance = camera.focusDistance();
    params.apertureRadius = camera.dofEnable() ? camera.focalLength() / camera.fstop() * 0.5f : 0.0f;
    params.exposure = camera.exposure();

    params.frameIndex = 0; // Must be constant for hasing

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
        params.blueNoise[i].texture = resources.get<GpuBindlessResource>(_blueNoiseBindlessResourceIds[i]).handle;
    for(unsigned int i = 0; i < HALTON_SAMPLE_COUNT; ++i)
        params.halton[i] = _halton[i];

    params.backgroundQuat = scene.sky()->quaternion();
    params.backgroundExposure = scene.sky()->exposure();

    const std::vector<std::shared_ptr<Object>>& objects = scene.objects();
    std::vector<GpuInstance> gpuInstances;
    gpuInstances.reserve(objects.size());

    std::vector<GLuint> gpuEmitters;

    for(std::size_t i = 0; i < objects.size(); ++i)
    {
        std::shared_ptr<Object> object = objects[i];
        GpuInstance& gpuInstance = gpuInstances.emplace_back();
        gpuInstance.albedo = glm::vec4(object->material()->defaultAlbedo(), 1.0);
        gpuInstance.emission = glm::vec4(object->material()->defaultEmissionColor()
                                         * object->material()->defaultEmissionLuminance(),
                                         1.0);
        gpuInstance.specular = glm::vec4(
                    // Roughness to GGX's 'a' parameter
                    object->material()->defaultRoughness() * object->material()->defaultRoughness(),
                    object->material()->defaultMetalness(),
                    object->material()->defaultReflectance(),
                    0);
        gpuInstance.radius = object->mesh()->radius();
        gpuInstance.materialId = _objectToMat[i];
        gpuInstance.primitiveType = object->mesh()->primitiveType();
        if(object->body().get() != nullptr)
        {
            gpuInstance.mass = object->body()->mass();
            gpuInstance.position = glm::dvec4(glm::vec3(object->body()->position()), 1);
            gpuInstance.quaternion = glm::vec4(quatConjugate(object->body()->quaternion()));
        }
        else
        {
            gpuInstance.mass = 0.0f;
            gpuInstance.position = glm::dvec4(0, 0, 0, 1);
            gpuInstance.quaternion = glm::vec4(0, 0, 1, 0);
        }

        if(glm::any(glm::greaterThan(object->material()->defaultEmissionColor(), glm::vec3())))
        {
            gpuEmitters.push_back(i);
        }
    }

    std::vector<GpuDirectionalLight> gpuDirectionalLights;
    gpuDirectionalLights.reserve(scene.sky()->directionalLights().size());
    auto addGpuDirectionalLights = [&](const std::vector<std::shared_ptr<DirectionalLight>>& lights)
    {
        for(std::size_t i = 0; i < lights.size(); ++i)
        {
            const std::shared_ptr<DirectionalLight>& directionalLight = lights[i];
            GpuDirectionalLight& gpuDirectionalLight = gpuDirectionalLights.emplace_back();
            gpuDirectionalLight.directionCosThetaMax = glm::vec4(
                        directionalLight->direction(),
                        1 - directionalLight->solidAngle() / (2 * glm::pi<float>()));
            gpuDirectionalLight.emissionSolidAngle = glm::vec4(
                        directionalLight->emissionColor() * directionalLight->emissionLuminance(),
                        directionalLight->solidAngle());
        }
    };

    addGpuDirectionalLights(scene.sky()->directionalLights());


    std::size_t frameHash = std::hash<std::string_view>()(std::string_view((char*)&params, sizeof (CommonParams)));
    frameHash ^= std::hash<std::string_view>()(std::string_view((char*)gpuInstances.data(), sizeof (GpuInstance) * gpuInstances.size()));
    frameHash ^= std::hash<std::string_view>()(std::string_view((char*)gpuDirectionalLights.data(), sizeof (GpuDirectionalLight) * gpuDirectionalLights.size()));

    if(frameHash != _lastFrameHash)
        _frameIndex = 0;
    else
        ++_frameIndex;

    if(_frameIndex >= MAX_FRAME_COUNT)
        return;

    _lastFrameHash = frameHash;
    params.frameIndex = _frameIndex;

    GLuint pathTraceTexId = resources.get<GpuImageResource>(ResourceName(PathTrace)).texId;

    {
        ProfileGpu(PathTrace);

        glUseProgram(_computePathTraceProgramId);

        // Set uniforms for sub-systems
        GLuint textureUnitStart = 0;
        resources.setPathTracerResources(context, _computePathTraceProgramId, textureUnitStart);

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, _commonUbo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(CommonParams), &params, GL_STREAM_DRAW);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _instancesSSBO);
        GLsizei instancesSize = gpuInstances.size() * sizeof(GpuInstance);
        glBufferData(GL_SHADER_STORAGE_BUFFER, instancesSize, gpuInstances.data(), GL_STREAM_DRAW);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _dirLightsSSBO);
        GLsizei dirLightsSize = gpuDirectionalLights.size() * sizeof(GpuDirectionalLight);
        glBufferData(GL_SHADER_STORAGE_BUFFER, dirLightsSize, gpuDirectionalLights.data(), GL_STREAM_DRAW);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _emittersSSBO);
        GLsizei emittersSize = gpuEmitters.size() * sizeof(decltype (gpuEmitters.front()));
        glBufferData(GL_SHADER_STORAGE_BUFFER, emittersSize, gpuEmitters.data(), GL_STREAM_DRAW);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _materialsSSBO);

        glBindImageTexture(_pathTraceUnit, pathTraceTexId, 0, false, 0, GL_WRITE_ONLY, _pathTraceFormat);

        glDispatchCompute((_viewport.width + 7) / 8, (_viewport.height + 3) / 4, 1);
    }

    {
        ProfileGpu(Clear);

        glViewport(0, 0, _viewport.width, _viewport.height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    {
        ProfileGpu(ColorGrade);

        glUseProgram(_colorGradingId);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pathTraceTexId);

        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
    }

    glUseProgram(0);

    glFlush();
}

}
