#include "scene.h"

#include <imgui/imgui.h>

#include "../resource/instance.h"

#include "sky.h"
#include "terrain.h"
#include "materialdatabase.h"


namespace unisim
{


Scene::Scene(const std::string& name) :
    _name(name),
    _sky(new SkySphere()),
    _terrain(new NoTerrain()),
    _materials(new MaterialDatabase())
{
}

Scene::~Scene()
{

}

void Scene::ui()
{
    if(ImGui::TreeNode("Sky"))
    {
        if(_sky)
            _sky->ui();
        else
            ImGui::Text("No Sky");

        ImGui::TreePop();
    }

    if(ImGui::TreeNode("Terrain"))
    {
        if(_terrain)
            _terrain->ui();
        else
            ImGui::Text("No Terrain");

        ImGui::TreePop();
    }

    if(ImGui::TreeNode("Instances"))
    {
        for(const auto& instance : _instances)
        {
            if(ImGui::TreeNode(instance->name().c_str()))
            {
                instance->ui();
                ImGui::TreePop();
            }
        }

        if(_instances.empty())
            ImGui::Text("No Instances");

        ImGui::TreePop();
    }
}

}
