#ifdef UNISIM_GRAPHIC_BACKEND_GL

#include "window.h"

#include <GLFW/glfw3.h>

#include <PilsCore/Utils/Assert.h>

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

void Window::resetViewportNative()
{

}

void Window::resizeViewportNative()
{

}

bool Window::newFrameNative()
{
    return true;
}

bool Window::presentNative()
{
    PILS_ASSERT(_glfwWindow != nullptr, "GLFW window pointer is null");
    glfwSwapBuffers(_glfwWindow);
    return _glfwWindow != nullptr;
}

void Window::closeNative()
{
}

}

#endif // UNISIM_GRAPHIC_BACKEND_GL
