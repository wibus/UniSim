#include "lighttask.h"

#include <imgui/imgui.h>

#include "GLM/gtc/constants.hpp"

#include "../system/profiler.h"

#include "../resource/light.h"
#include "../resource/material.h"
#include "../resource/instance.h"
#include "../resource/primitive.h"
#include "../resource/sky.h"

#include "../graphic/gpudevice.h"

#include "../scene.h"


namespace unisim
{

DefineProfilePoint(Lighting);

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

LightTask::LightTask() :
    PathTracerProviderTask("Light")
{
}

glm::vec3 toLinear(const glm::vec3& sRGB)
{
    glm::bvec3 cutoff = glm::lessThan(sRGB, glm::vec3(0.04045));
    glm::vec3 higher = glm::pow((sRGB + glm::vec3(0.055))/glm::vec3(1.055), glm::vec3(2.4));
    glm::vec3 lower = sRGB/glm::vec3(12.92);

    return glm::mix(higher, lower, cutoff);
}

bool LightTask::defineResources(GraphicContext& context)
{
    bool ok = true;

    GpuResourceManager& resources = context.resources;

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

bool LightTask::definePathTracerInterface(
    GraphicContext& context,
    PathTracerInterface& interface)
{
    bool ok = true;

    ok = ok && interface.declareStorage({"Emitters"});
    ok = ok && interface.declareStorage({"DirectionalLights"});

    return ok;
}

void LightTask::bindPathTracerResources(
    GraphicContext &context,
    CompiledGpuProgramInterface& compiledGpi) const
{
    GpuResourceManager& resources = context.resources;

    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(Emitters)),          compiledGpi.getStorageBindPoint("Emitters"));
    context.device.bindBuffer(resources.get<GpuStorageResource>(ResourceName(DirectionalLights)), compiledGpi.getStorageBindPoint("DirectionalLights"));
}

void LightTask::update(GraphicContext& context)
{
    Profile(Lighting);

    GpuResourceManager& resources = context.resources;

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

uint64_t LightTask::toGpu(
    GraphicContext& context,
        std::vector<GpuEmitter>& gpuEmitters,
        std::vector<GpuDirectionalLight>& gpuDirectionalLights)
{
    // Emitters
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

    // Directional lights
    gpuDirectionalLights.reserve(2);
    auto processDirectionalLight = [&](DirectionalLight& light)
    {
        GpuDirectionalLight& gpuDirectionalLight = gpuDirectionalLights.emplace_back();
        gpuDirectionalLight.directionCosThetaMax = glm::vec4(
            light.direction(),
            1 - light.solidAngle() / (2 * glm::pi<float>()));
        gpuDirectionalLight.emissionSolidAngle = glm::vec4(
            light.emissionColor() * light.emissionLuminance(),
            light.solidAngle());
    };

    if (const Atmosphere* atmosphere = context.scene.sky()->atmosphere().get())
    {
        processDirectionalLight(*atmosphere->sun());
        processDirectionalLight(*atmosphere->moon());
    }


    // Finalize
    uint64_t hash = 0;
    hash = hashVec(gpuEmitters, hash);
    hash = hashVec(gpuDirectionalLights, hash);

    return hash;
}

}
