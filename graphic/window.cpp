#include "window.h"

#include <GLFW/glfw3.h>

#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include <PilsCore/Utils/Assert.h>
#include <PilsCore/Utils/Logger.h>

#include "../system/input.h"


namespace unisim
{

WindowRegistry& WindowRegistry::getInstance()
{
    static WindowRegistry g_Instance;

    return g_Instance;
}

void WindowRegistry::registerWindow(Window* window, uint64_t handle)
{
    _windows.insert(std::make_pair(handle, window));
}

void WindowRegistry::unregisterWindow(uint64_t handle)
{
    _windows.erase(handle);
}

Window* WindowRegistry::getWindow(uint64_t handle)
{
    auto it = _windows.find(handle);
    if (it != _windows.end())
        return it->second;

    return nullptr;
}

WindowEventListener::WindowEventListener()
{
}

WindowEventListener::~WindowEventListener()
{
}

void WindowEventListener::onWindowResize(const Window& window, int width, int height)
{

}

void WindowEventListener::onWindowKeyboard(const Window& window, const KeyboardEvent& event)
{

}

void WindowEventListener::onWindowMouseMove(const Window& window, const MouseMoveEvent& event)
{

}

void WindowEventListener::onWindowMouseButton(const Window& window, const MouseButtonEvent& event)
{

}

void WindowEventListener::onWindowMouseScroll(const Window& window, const MouseScrollEvent& event)
{

}


void glfWHandleResize(GLFWwindow* window, int width, int height)
{
    uint64_t handle = reinterpret_cast<uint64_t>(window);
    if (Window* window = WindowRegistry::getInstance().getWindow(handle))
    {
        window->onResize(width, height);
    }
}

void glfWHandleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    uint64_t handle = reinterpret_cast<uint64_t>(window);
    if (Window* window = WindowRegistry::getInstance().getWindow(handle))
    {
        KeyboardEvent event = {key, scancode, action, mods};
        window->onKeyboard(event);
    }
}

void glfWHandleMouseMove(GLFWwindow* window, double xpos, double ypos)
{
    uint64_t handle = reinterpret_cast<uint64_t>(window);
    if (Window* window = WindowRegistry::getInstance().getWindow(handle))
    {
        MouseMoveEvent event = {xpos, ypos};
        window->onMouseMove(event);
    }
}

void glfWHandleMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    uint64_t handle = reinterpret_cast<uint64_t>(window);
    if (Window* window = WindowRegistry::getInstance().getWindow(handle))
    {
        MouseButtonEvent event = {button, action, mods};
        window->onMouseButton(event);
    }
}

void glfWHandleScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    uint64_t handle = reinterpret_cast<uint64_t>(window);
    if (Window* window = WindowRegistry::getInstance().getWindow(handle))
    {
        MouseScrollEvent event = {xoffset, yoffset};
        window->onMouseScroll(event);
    }
}

Window::Window() :
    Window(1280, 720)
{
}

Window::Window(int requestedWidth, int requestedHeight) :
    _glfwWindow(nullptr),
    _width(-1),
    _height(-1)
{
    glfwSetErrorCallback([](int error, const char* description)
    {
        PILS_ERROR("GLFW Error ", error, ": ",  description, "%s\n");
    });

    if (!glfwInit())
        return;

    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

    int monitorScore = -1;
    const int SMALL_MONITOR_SCORE = 1;
    const int LARGE_MONITOR_SCORE = 0;

    int viewportW = requestedWidth;
    int viewportH = requestedHeight;
    int viewportX = 0, viewportY = 0;
    GLFWmonitor* chosenMonitor = nullptr;

    for(int i = 0; i < monitorCount; ++i)
    {
        int xpos, ypos, width, height;
        glfwGetMonitorWorkarea(monitors[i], &xpos, &ypos, &width, &height);

        if(width < requestedWidth || height < requestedHeight)
        {
            if(monitorScore < SMALL_MONITOR_SCORE)
            {
                chosenMonitor = monitors[i];
                monitorScore = SMALL_MONITOR_SCORE;

                viewportW = width;
                viewportH = height;
                viewportX = xpos;
                viewportY = ypos;
            }
        }
        else
        {
            if(monitorScore < LARGE_MONITOR_SCORE)
            {
                chosenMonitor = monitors[i];
                monitorScore = LARGE_MONITOR_SCORE;

                viewportW = requestedWidth;
                viewportH = requestedHeight;
                viewportX = xpos + (width - viewportW) / 2;
                viewportY = ypos + (height - viewportH) / 2;
            }
        }
    }

    if(!chosenMonitor)
    {
        return;
    }

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

    _width = viewportW;
    _height = viewportH;
    _glfwWindow = glfwCreateWindow(_width, _height, "Universe", NULL, NULL);

    if (!_glfwWindow)
    {

        glfwTerminate();
        return;
    }

    WindowRegistry::getInstance().registerWindow(this, handle());

    glfwSetWindowPos(_glfwWindow, viewportX, viewportY);
    glfwMakeContextCurrent(_glfwWindow);

    glewExperimental = GL_TRUE;
    glewInit();

    glfwSwapInterval(1);

    glfwSetWindowSizeCallback(_glfwWindow, glfWHandleResize);
    glfwSetKeyCallback(_glfwWindow, glfWHandleKeyboard);
    glfwSetCursorPosCallback(_glfwWindow, glfWHandleMouseMove);
    glfwSetMouseButtonCallback(_glfwWindow, glfWHandleMouseButton);
    glfwSetScrollCallback(_glfwWindow, glfWHandleScroll);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(_glfwWindow, true);
    const char* glsl_version = "#version 440";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

Window::~Window()
{
    if (_glfwWindow != nullptr)
    {
        WindowRegistry::getInstance().unregisterWindow(reinterpret_cast<uint64_t>(_glfwWindow));
    }

    PILS_ASSERT(_eventListeners.empty(), "Window is being destroyed with active listeners");
}

bool Window::isValid() const
{
    return _glfwWindow != nullptr;
}

uint64_t Window::handle() const
{
    return reinterpret_cast<uint64_t>(_glfwWindow);
}

void Window::registerEventListener(WindowEventListener* listener)
{
    if (listener != nullptr)
        _eventListeners.insert(listener);
}

void Window::unregisterEventListener(WindowEventListener* listener)
{
    if (listener != nullptr)
        _eventListeners.erase(listener);
}

void Window::onResize(int width, int height)
{
    _width = width;
    _height = height;

    for(auto listener : _eventListeners)
        listener->onWindowResize(*this, width, height);
}

void Window::onKeyboard(const KeyboardEvent& event)
{
    for(auto listener : _eventListeners)
        listener->onWindowKeyboard(*this, event);
}

void Window::onMouseMove(const MouseMoveEvent& event)
{
    for(auto listener : _eventListeners)
        listener->onWindowMouseMove(*this, event);
}

void Window::onMouseButton(const MouseButtonEvent& event)
{
    for(auto listener : _eventListeners)
        listener->onWindowMouseButton(*this, event);
}

void Window::onMouseScroll(const MouseScrollEvent& event)
{
    for(auto listener : _eventListeners)
        listener->onWindowMouseScroll(*this, event);
}

void Window::ImGuiNewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

bool Window::shouldClose()
{
    PILS_ASSERT(_glfwWindow != nullptr, "GLFW window pointer is null");
    return glfwWindowShouldClose(_glfwWindow);
}

void Window::pollEvents()
{
    glfwPollEvents();
}

void Window::present()
{
    PILS_ASSERT(_glfwWindow != nullptr, "GLFW window pointer is null");
    glfwSwapBuffers(_glfwWindow);
}

void Window::close()
{
    glfwTerminate();
}

}
