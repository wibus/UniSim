#include "pathtracerui.h"

#include <imgui/imgui.h>

#include "../scene.h"


namespace unisim
{

PathTracerUi::PathTracerUi() :
    Ui(true)
{

}

PathTracerUi::~PathTracerUi()
{

}

void PathTracerUi::render(Scene& scene)
{
    if(!_showUi)
        return;

    if(ImGui::Begin("Path Tracer", &_showUi))
    {

    }
    ImGui::End();
}

}
