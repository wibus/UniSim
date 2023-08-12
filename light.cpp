#include "light.h"

#include <imgui/imgui.h>

#include "GLM/gtc/constants.hpp"

#include "sky.h"
#include "scene.h"
#include "instance.h"
#include "primitive.h"
#include "material.h"
#include "profiler.h"


namespace unisim
{

DefineProfilePoint(Lighting);

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

void DirectionalLight::ui()
{
    glm::vec3 direction = _position;
    if(ImGui::SliderFloat3("Direction", &direction[0], -1, 1))
        setDirection(glm::normalize(direction));

    glm::vec3 emissionColor = _emissionColor;
    if(ImGui::ColorPicker3("Emission Color", &emissionColor[0]))
        setEmissionColor(emissionColor);

    float emissionLuminance = _emissionLuminance;
    if(ImGui::InputFloat("Emission Luminance", &emissionLuminance))
        setEmissionLuminance(glm::max(0.0f, emissionLuminance));

    float solidAngle = _solidAngle;
    if(ImGui::InputFloat("Solid Angle", &solidAngle, 0.0f, 0.0f, "%.6f"))
        setSolidAngle(glm::clamp(solidAngle, 0.0f ,4 * glm::pi<float>()));
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
    Profile(Lighting);

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
    const auto& instances = context.scene.instances();
    for(std::size_t i = 0; i < instances.size(); ++i)
    {
        const Instance& instance = *instances[i];
        for(std::size_t p = 0; p < instance.primitives().size(); ++p)
        {
            const Primitive& primitive = *instance.primitives()[p];
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
