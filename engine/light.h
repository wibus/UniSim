#ifndef LIGHTING_H
#define LIGHTING_H

#include <vector>
#include <string>

#include <GLM/glm.hpp>

#include "graphictask.h"



namespace unisim
{

class Scene;

struct GpuEmitter;
struct GpuDirectionalLight;


class DirectionalLight
{
public:
    DirectionalLight(const std::string& name);
    ~DirectionalLight();

    const std::string& name() const { return _name; }

    glm::vec3 direction() const { return _position; }
    void setDirection(const glm::vec3& position) { _position = position; }

    glm::vec3 emissionColor() const { return _emissionColor; }
    void setEmissionColor(const glm::vec3& color) { _emissionColor = color; }

    float emissionLuminance() const { return _emissionLuminance; }
    void setEmissionLuminance(float luminance) { _emissionLuminance = luminance; }

    float solidAngle() const { return _solidAngle; }
    void setSolidAngle(float solidAngle) { _solidAngle = solidAngle; }

    void ui();

private:
    std::string _name;
    glm::vec3 _position;
    glm::vec3 _emissionColor;
    float _emissionLuminance;
    float _solidAngle;
};


class Lighting : public GraphicTask
{
public:
    Lighting();

    bool definePathTracerInterface(GraphicContext& context, PathTracerInterface& interface) override;

    bool defineResources(GraphicContext& context) override;
    
    void bindPathTracerResources(
        GraphicContext& context,
            PathTracerInterface& interface) const override;

    void update(GraphicContext& context) override;

private:
    uint64_t toGpu(
        GraphicContext& context,
            std::vector<GpuEmitter>& gpuEmitters,
            std::vector<GpuDirectionalLight>& gpuDirectionalLights);
};

}

#endif // LIGHTING_H
