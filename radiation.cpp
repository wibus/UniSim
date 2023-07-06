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


namespace unisim
{

DeclareProfilePointGpu(Radiation);
DeclareProfilePointGpu(Clear);
DeclareProfilePointGpu(PathTrace);
DeclareProfilePointGpu(ColorGrade);

DeclareResource(SkyMap);
DefineResource(PathTrace);

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

    float pad1;
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

std::string loadSource(const std::string& fileName)
{
    std::ifstream t(fileName);
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());
    return str;
}

void _print_shader_info_log(GLuint shaderId)
{
    int max_length = 2048;
    int actual_length = 0;
    char shader_log[2048];
    glGetShaderInfoLog(shaderId, max_length, &actual_length, shader_log);
    std::cout << "shader info log for GL index " << shaderId << ":" << std::endl;
    std::cout << shader_log << std::endl;
}

bool compileShader(GLuint shaderId, const std::string& name)
{
    printf("Compiling shader %s\n" , name.c_str());

    glCompileShader(shaderId);

    // check for compile errors
    int params = -1;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &params);

    if (GL_TRUE != params)
    {
        fprintf(stderr, "ERROR: GL shader index %i did not compile\n", shaderId);
        _print_shader_info_log(shaderId);
        return false; // or exit or something
    }

    return true;
}

void printProgrammeInfoLog(GLuint programId)
{
    int max_length = 2048;
    int actual_length = 0;
    char program_log[2048];
    glGetProgramInfoLog(programId, max_length, &actual_length, program_log);
    std::cout << "program info log for GL index " << programId << ":" << std::endl;
    std::cout << program_log << std::endl;
}

bool validateProgram(GLuint programId, const std::string& name)
{
    printf("Validating program %s\n" , name.c_str());

    int linkStatus = -1;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

    glValidateProgram(programId);

    int validationStatus = -1;
    glGetProgramiv(programId, GL_VALIDATE_STATUS, &validationStatus);
    printf("program %i GL_VALIDATE_STATUS = %i\n", programId, validationStatus);
    if (GL_TRUE != validationStatus)
    {
        printProgrammeInfoLog(programId);
        return false;
    }

    return true;
}

bool generateGraphicProgram(GLuint& programId, const std::string& vertexFileName, const std::string& fragmentFileName)
{
    std::string vertexSrc = loadSource(vertexFileName);
    const char* vertexSrcChar = vertexSrc.data();
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexSrcChar, NULL);
    if(!compileShader(vs, vertexFileName))
        return false;

    std::string fragmentSrc = loadSource(fragmentFileName);
    const char* fragmentSrcChar = fragmentSrc.data();
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentSrcChar, NULL);
    if(!compileShader(fs, fragmentFileName))
        return false;

    programId = glCreateProgram();
    glAttachShader(programId, fs);
    glAttachShader(programId, vs);
    glLinkProgram(programId);

    if(!validateProgram(programId, vertexFileName + " - " + fragmentFileName))
        return false;

    return true;
}

bool generateComputeProgram(GLuint& programId, const std::string& computeFileName)
{
    std::string computeSrc = loadSource(computeFileName);
    const char* computeSrcChar = computeSrc.data();
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, 1, &computeSrcChar, NULL);
    if(!compileShader(cs, computeFileName))
        return false;

    programId = glCreateProgram();
    glAttachShader(programId, cs);
    glLinkProgram(programId);

    if(!validateProgram(programId, computeFileName))
        return false;

    return true;
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

    bool ok = generateComputeProgram(_computePathTraceId, "shaders/pathtrace.glsl");
    ok = generateGraphicProgram(_colorGradingId, "shaders/fullscreen.vert", "shaders/colorgrade.frag") && ok;

    if(!ok)
        return ok;

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
    _pathTraceLoc = glGetUniformLocation(_computePathTraceId, "result");
    glProgramUniform1i(_computePathTraceId, _pathTraceLoc, _pathTraceUnit);

    _backgroundUnit = 0;
    _backgroundLoc = glGetUniformLocation(_computePathTraceId, "backgroundImg");

    return ok;
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
        gpuInstance.position = glm::dvec4(object->body()->position(), 1);
        gpuInstance.quaternion = glm::vec4(quatConjugate(object->body()->quaternion()));
        gpuInstance.radius = object->mesh()->radius();
        gpuInstance.mass = object->body()->mass();
        gpuInstance.materialId = _objectToMat[i];

        if(glm::any(glm::greaterThan(object->material()->defaultEmissionColor(), glm::vec3())))
        {
            gpuEmitters.push_back(i);
        }
    }


    const std::vector<std::shared_ptr<DirectionalLight>>& directionalLights = scene.directionalLights();
    std::vector<GpuDirectionalLight> gpuDirectionalLights;
    gpuDirectionalLights.reserve(directionalLights.size());
    for(std::size_t i = 0; i < directionalLights.size(); ++i)
    {
        std::shared_ptr<DirectionalLight> directionalLight = directionalLights[i];
        GpuDirectionalLight& gpuDirectionalLight = gpuDirectionalLights.emplace_back();
        gpuDirectionalLight.directionCosThetaMax = glm::vec4(
                    directionalLight->direction(),
                    1 - directionalLight->solidAngle() / (2 * glm::pi<float>()));
        gpuDirectionalLight.emissionSolidAngle = glm::vec4(
                    directionalLight->emissionColor() * directionalLight->emissionLuminance(),
                    directionalLight->solidAngle());
    }


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

        glUseProgram(_computePathTraceId);

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

        glUniform1i(_backgroundLoc, _backgroundUnit);
        glActiveTexture(GL_TEXTURE0 + _backgroundUnit);
        glBindTexture(GL_TEXTURE_2D, resources.get<GpuTextureResource>(ResourceName(SkyMap)).texId);

        glBindImageTexture(_pathTraceUnit, pathTraceTexId, 0, false, 0, GL_WRITE_ONLY, _pathTraceFormat);

        glDispatchCompute((camera.viewport().width + 7) / 8, (camera.viewport().width + 3) / 4, 1);
    }

    {
        ProfileGpu(Clear);

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
