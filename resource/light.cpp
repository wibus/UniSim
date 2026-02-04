#include "light.h"

#include <imgui/imgui.h>

#include "GLM/gtc/constants.hpp"


namespace unisim
{

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

}
