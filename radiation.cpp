#include "radiation.h"

#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <functional>
#include <string_view>

#include "GLM/gtc/constants.hpp"

#include "object.h"
#include "body.h"
#include "mesh.h"
#include "camera.h"
#include "units.h"
#include "material.h"
#include "random.h"


namespace unisim
{

struct GpuInstance
{
    glm::vec4 albedo;
    glm::vec4 emission;
    glm::vec4 position;
    glm::vec4 quaternion;

    float radius;
    float mass;

    uint materialId;

    float pad1;
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
    glm::mat4 invViewMat;
    glm::mat4 rayMatrix;
    GLuint instanceCount;
    GLfloat radiusScale;
    GLfloat exposure;
    GLuint emitterStart;
    GLuint emitterEnd;
    GLuint frameIndex;

    GLuint pad1;
    GLuint pad2;

    // Background
    glm::vec4 backgroundQuat;
    GPUBindlessTexture backgroundImg;

    GPUBindlessTexture blueNoise[Radiation::BLUE_NOISE_TEX_COUNT];

    glm::vec2 halton[Radiation::HALTON_SAMPLE_COUNT];
};

const int MAX_OBJECT_COUNT = 100;

Radiation::Radiation() :
    _frameIndex(0),
    _lastFrameHash(0)
{
    for(unsigned int i = 0; i < HALTON_SAMPLE_COUNT; ++i)
    {
        double* sample = halton(i, 2);
        _halton[i] = glm::vec2(sample[0], sample[1]);
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

void _print_programme_info_log(GLuint programId)
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
        _print_programme_info_log(programId);
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

GLuint generateImageTexture(const Texture& texture)
{
    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0, // mip level
        texture.numComponents == 3 ? GL_RGB8 : GL_RGBA8,
        texture.width,
        texture.height,
        0, // border
        texture.numComponents == 3 ? GL_RGB : GL_RGBA,
        GL_UNSIGNED_BYTE,
        texture.data);

    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

GLuint generateUAV()
{
    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    return textureID;
}

void updateUAV(GLuint textureID, int width, int height, GLint format)
{
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

    glBindTexture(GL_TEXTURE_2D, 0);
}

GpuMaterial createGpuMaterial(const std::shared_ptr<Material>& material)
{
    GLuint64 alebdoHdl = 0;

    if(material->albedo() != nullptr)
    {
        GLuint albedoId = generateImageTexture(*material->albedo());
        alebdoHdl = glGetImageHandleARB(albedoId, 0, GL_FALSE, 0, GL_RGBA8);
        glMakeImageHandleResidentARB(alebdoHdl, GL_READ_ONLY);
    }

    return {GPUBindlessTexture(alebdoHdl), GPUBindlessTexture(0)};
}

bool Radiation::initialize(const std::vector<std::shared_ptr<Object>>& objects, const Viewport& viewport)
{
    Material backgroundMat("Background");
    if(backgroundMat.loadAlbedo("textures/background.jpg"))
    {
        _backgroundTexId = generateImageTexture(*backgroundMat.albedo());
        _backgroundHdl = glGetImageHandleARB(_backgroundTexId, 0, GL_FALSE, 0, GL_RGBA8);
        glMakeImageHandleResidentARB(_backgroundHdl, GL_READ_ONLY);
    }
    else
    {
        std::cerr << "Cannot load sky texture. Did you install the texture pack?" << std::endl;
        _backgroundTexId = 0;
        _backgroundHdl = 0;
    }

    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
    {
        std::stringstream ss;
        ss << "LDR_RGBA_" << i << ".png";
        std::string blueNoiseName = ss.str();

        Material blueNoiseMat(blueNoiseName);
        if(blueNoiseMat.loadAlbedo("textures/bluenoise64/" + blueNoiseName))
        {
            _blueNoiseTexIds[i] = generateImageTexture(*blueNoiseMat.albedo());
            _blueNoiseTexHdls[i] = glGetImageHandleARB(_blueNoiseTexIds[i], 0, GL_FALSE, 0, GL_RGBA8);
            glMakeImageHandleResidentARB(_blueNoiseTexHdls[i], GL_READ_ONLY);
        }
        else
        {
            std::cerr << "Cannot load blue noise texture. Did you install the texture pack?" << std::endl;
            _blueNoiseTexIds[i] = 0;
            _blueNoiseTexHdls[i] = 0;
        }
    }

    std::vector<GpuMaterial> gpuMaterials;

    _objectToMat.resize(objects.size());
    for(std::size_t i = 0; i < objects.size(); ++i)
    {
        if(objects[i]->material().get() != nullptr && objects[i]->material()->albedo())
        {
            GpuMaterial gpuMat = createGpuMaterial(objects[i]->material());
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

    glGenBuffers(1, &_instancesUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, _instancesUbo);
    glBufferData(GL_UNIFORM_BUFFER, MAX_OBJECT_COUNT * sizeof(GpuInstance), nullptr, GL_STREAM_DRAW);

    glGenBuffers(1, &_materialsUbo);
    glBindBuffer(GL_UNIFORM_BUFFER, _materialsUbo);
    glBufferData(GL_UNIFORM_BUFFER, gpuMaterials.size() * sizeof(GpuMaterial), gpuMaterials.data(), GL_STREAM_DRAW);

    bool ok = generateComputeProgram(_computePathTraceId, "shaders/pathtrace.glsl");
    ok = generateGraphicProgram(_colorGradingId, "shaders/fullscreen.vert", "shaders/colorgrade.frag") && ok;

    _pathTraceFormat = GL_RGBA16F;
    _pathTraceUAVId = generateUAV();
    updateUAV(_pathTraceUAVId, viewport.width, viewport.height, _pathTraceFormat);

    _pathTraceUnit = 0;
    _pathTraceLoc = glGetUniformLocation(_computePathTraceId, "result");
    glProgramUniform1i(_computePathTraceId, _pathTraceLoc, _pathTraceUnit);


    return ok;
}

void Radiation::setViewport(Viewport viewport)
{
    updateUAV(_pathTraceUAVId, viewport.width, viewport.height, _pathTraceFormat);
}

glm::vec3 toLinear(const glm::vec3& sRGB)
{
    glm::bvec3 cutoff = glm::lessThan(sRGB, glm::vec3(0.04045));
    glm::vec3 higher = glm::pow((sRGB + glm::vec3(0.055))/glm::vec3(1.055), glm::vec3(2.4));
    glm::vec3 lower = sRGB/glm::vec3(12.92);

    return glm::mix(higher, lower, cutoff);
}

void Radiation::draw(const std::vector<std::shared_ptr<Object>>& objects, double dt, const Camera &camera)
{
    glUseProgram(_computePathTraceId);

    glm::dmat4 view = camera.view();
    glm::mat4 proj = camera.proj();
    glm::mat4 screen = camera.screen();
    glm::mat4 viewToScreen = screen * proj;

    CommonParams params;
    params.invViewMat = glm::inverse(view);
    params.rayMatrix = glm::inverse(viewToScreen);
    params.instanceCount = objects.size();
    params.radiusScale = 1;
    params.exposure = camera.exposure();
    params.emitterStart = 0;
    params.emitterEnd = 1;
    params.frameIndex = 0; // Must be constant for hasing
    params.backgroundQuat = quatConjugate(EARTH_BASE_QUAT);
    params.backgroundImg.texture = _backgroundHdl;
    for(unsigned int i = 0; i < BLUE_NOISE_TEX_COUNT; ++i)
        params.blueNoise[i].texture = _blueNoiseTexHdls[i];
    for(unsigned int i = 0; i < HALTON_SAMPLE_COUNT; ++i)
        params.halton[i] = _halton[i];

    std::vector<GpuInstance> gpuInstances;
    gpuInstances.reserve(MAX_OBJECT_COUNT);
    for(std::size_t i = 0; i < objects.size(); ++i)
    {
        std::shared_ptr<Object> object = objects[i];
        GpuInstance& gpuInstance = gpuInstances.emplace_back();
        gpuInstance.albedo = glm::vec4(object->material()->defaultAlbedo(), 1.0);
        gpuInstance.emission = glm::vec4(object->material()->defaultEmission(), 1.0);
        gpuInstance.position = view * glm::dvec4(object->body()->position(), 1);
        gpuInstance.quaternion = glm::vec4(quatConjugate(object->body()->quaternion()));
        gpuInstance.radius = object->mesh()->radius();
        gpuInstance.mass = object->body()->mass();
        gpuInstance.materialId = _objectToMat[i];
    }

    std::size_t frameHash = std::hash<std::string_view>()(std::string_view((char*)&params, sizeof (CommonParams)));
    //frameHash ^= std::hash<std::string_view>()(std::string_view((char*)gpuInstances.data(), sizeof (GpuInstance) * gpuInstances.size()));

    if(frameHash != _lastFrameHash)
        _frameIndex = 0;
    else
        ++_frameIndex;

    if(_frameIndex >= MAX_FRAME_COUNT)
        return;

    _lastFrameHash = frameHash;
    params.frameIndex = _frameIndex;

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, _commonUbo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CommonParams), &params, GL_STREAM_DRAW);

    glBindBufferBase(GL_UNIFORM_BUFFER, 1, _instancesUbo);
    glBufferData(GL_UNIFORM_BUFFER, MAX_OBJECT_COUNT * sizeof(GpuInstance), gpuInstances.data(), GL_STREAM_DRAW);

    glBindBufferBase(GL_UNIFORM_BUFFER, 2, _materialsUbo);

    glBindImageTexture(_pathTraceUnit, _pathTraceUAVId, 0, false, 0, GL_WRITE_ONLY, _pathTraceFormat);

    glDispatchCompute((camera.viewport().width + 7) / 8, (camera.viewport().width + 3) / 4, 1);



    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(_colorGradingId);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _pathTraceUAVId);

    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 3);

    glUseProgram(0);

    glFlush();
}

}
