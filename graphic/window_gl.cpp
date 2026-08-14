#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include "window.h"

#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "graphic.h"


namespace unisim
{

class WindowSurface
{

};

std::vector<const char*> getUserProvidedVkInstanceExtensions()
{
    return {};
}

bool Window::InitGlfwNative()
{
    return true;
}

bool Window::CreateGlfwSurface()
{
    return true;
}

void Window::ImGuiInitNative()
{
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(_glfwWindow, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION__HEADER.c_str());
}

void Window::ImGuiNewFrameNative()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL
