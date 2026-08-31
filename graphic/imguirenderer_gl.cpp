#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include "imguirenderer.h"

#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "graphic.h"
#include "window.h"
#include "view.h"


namespace unisim
{

ImGuiNativeData::ImGuiNativeData()
{
}

ImGuiNativeData::~ImGuiNativeData()
{
}

void ImGuiRenderer::ImGuiInitNative()
{
    ImGui_ImplGlfw_InitForOpenGL(_view.window().glfwWindow(), true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION__HEADER.c_str());
}

void ImGuiRenderer::ImGuiNewFrameNative() const
{
    ImGui_ImplOpenGL3_NewFrame();
}

void ImGuiRenderer::renderNative(GpuDevice& gpuDevice, const View& view) const
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL