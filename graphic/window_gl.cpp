#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include "window.h"

#include <GLFW/glfw3.h>

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
    glfwMakeContextCurrent(_glfwWindow);
    glfwSwapInterval(1);
    return true;
}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL
