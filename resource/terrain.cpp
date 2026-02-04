#include "terrain.h"

#include <imgui/imgui.h>

#include "../system/units.h"

#include "../resource/body.h"
#include "../resource/instance.h"
#include "../resource/material.h"
#include "../resource/primitive.h"

namespace unisim
{

Terrain::Terrain(double baseHeight, const std::shared_ptr<Material>& material, float uvScale)
{
    _plane.reset(new Plane(uvScale));
    _plane->setMaterial(material);

    std::shared_ptr<Body> body(new Body(1.0f, 1.0f, true));
    body->setPosition(glm::vec3(0, 0, baseHeight));
    body->setQuaternion(quat(glm::vec3(0, 0, 1), 0.0f));

    std::vector<std::shared_ptr<Primitive>> primitives;
    primitives.push_back(std::dynamic_pointer_cast<Primitive>(_plane));
    std::shared_ptr<Instance> instance(new Instance("Terrain", body, primitives, nullptr));

    _instances.push_back(instance);
}

Terrain::~Terrain()
{

}

void Terrain::ui()
{
    for(const auto& instance : _instances)
    {
        if(ImGui::TreeNode(instance->name().c_str()))
        {
            instance->ui();
            ImGui::TreePop();
        }
    }
}



}
