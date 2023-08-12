#include "instance.h"

#include <imgui/imgui.h>

#include "GLM/gtc/constants.hpp"
#include "GLM/gtx/transform.hpp"

#include "units.h"

#include "body.h"
#include "primitive.h"


namespace unisim
{

Instance::Instance(
        const std::string& name,
        const std::shared_ptr<Body>& body,
        const std::vector<std::shared_ptr<Primitive>>& primitives,
        Instance* parent) :
    _parent(parent),
    _name(name),
    _body(body),
    _primitives(primitives)
{

}

Instance::~Instance()
{

}

void Instance::setBody(const std::shared_ptr<Body>& body)
{
    _body = body;
}

void Instance::ui()
{
    if(ImGui::TreeNode("Body"))
    {
        if(_body)
            _body->ui();
        else
            ImGui::Text("No Body");

        ImGui::TreePop();
    }

    if(ImGui::TreeNode("Primitives"))
    {
        for(std::size_t p = 0; p < _primitives.size(); ++p)
        {
            Primitive& primitive = *_primitives[p];

            if(ImGui::TreeNode(&primitive, "Primitive %u: %s", (uint)p, Primitive::Type_Names[primitive.type()]))
            {
                primitive.ui();

                ImGui::TreePop();
            }
        }

        if(_primitives.empty())
            ImGui::Text("No Primitive");

        ImGui::TreePop();
    }
}

}
