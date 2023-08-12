#include "ui.h"

#include <imgui/imgui.h>

#include <imgui/imgui_impl_opengl3.h>

#include "profiler.h"


namespace unisim
{

DefineProfilePoint(ImGui_Render);
DefineProfilePointGpu(ImGui_Render);


Ui::Ui() :
    GraphicTask("UI")
{

}

void Ui::render(GraphicContext& context)
{
    Profile(ImGui_Render);
    ProfileGpu(ImGui_Render);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}
