#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "window.h"

#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <PilsCore/Utils/Assert.h>

#include "graphic.h"


namespace unisim
{

void Window::ImGuiInitNative()
{
    PILS_ASSERT(false, "Not implemented!");
}

void Window::ImGuiNewFrameNative()
{
    PILS_ASSERT(false, "Not implemented!");
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK
