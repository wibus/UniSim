#include "solarsystemui.h"

#include <imgui/imgui.h>

#include "../scene.h"


namespace unisim
{

SolarSystemUi::SolarSystemUi() :
    Ui(true)
{

}

SolarSystemUi::~SolarSystemUi()
{

}

void SolarSystemUi::render(Scene& scene)
{
    if(!_showUi)
        return;

    if(ImGui::Begin("Solar System", &_showUi))
    {

    }
    ImGui::End();
}

}
