#include "light.h"

#include "GLM/gtc/constants.hpp"

#include "sky.h"
#include "scene.h"
#include "object.h"
#include "primitive.h"
#include "material.h"
#include "profiler.h"


namespace unisim
{

DefineResource(DirectionalLights);
DefineResource(Emitters);


DirectionalLight::DirectionalLight(const std::string& name) :
    _name(name),
    _emissionColor(1, 1, 1),
    _emissionLuminance(1)
{

}

DirectionalLight::~DirectionalLight()
{

}

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

Lighting::Lighting() :
    GraphicTask("Radiation")
{
}

glm::vec3 toLinear(const glm::vec3& sRGB)
{
    glm::bvec3 cutoff = glm::lessThan(sRGB, glm::vec3(0.04045));
    glm::vec3 higher = glm::pow((sRGB + glm::vec3(0.055))/glm::vec3(1.055), glm::vec3(2.4));
    glm::vec3 lower = sRGB/glm::vec3(12.92);

    return glm::mix(higher, lower, cutoff);
}

bool Lighting::defineResources(GraphicContext& context)
{
    bool ok = true;

    ResourceManager& resources = context.resources;

    std::vector<GpuEmitter> gpuEmitters;
    std::vector<GpuDirectionalLight> gpuDirectionalLights;

    _hash = toGpu(
        context,
        gpuEmitters,
        gpuDirectionalLights);

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

    return ok;
}

void Lighting::setPathTracerResources(
        GraphicContext &context,
        PathTracerInterface &interface) const
{
    ResourceManager& resources = context.resources;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("Emitters"),
                     resources.get<GpuStorageResource>(ResourceName(Emitters)).bufferId);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("DirectionalLights"),
                     resources.get<GpuStorageResource>(ResourceName(DirectionalLights)).bufferId);

}

void Lighting::update(GraphicContext& context)
{
    ResourceManager& resources = context.resources;

    std::vector<GpuEmitter> gpuEmitters;
    std::vector<GpuDirectionalLight> gpuDirectionalLights;

    uint64_t hash = toGpu(
        context,
        gpuEmitters,
        gpuDirectionalLights);

    if(_hash == hash)
        return;

    _hash = hash;

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
}

uint64_t Lighting::toGpu(
        GraphicContext& context,
        std::vector<GpuEmitter>& gpuEmitters,
        std::vector<GpuDirectionalLight>& gpuDirectionalLights)
{
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
    hash = hashVec(gpuEmitters, hash);
    hash = hashVec(gpuDirectionalLights, hash);

    return hash;
}

}
