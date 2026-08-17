#ifndef WINDOW_H
#define WINDOW_H

#include <map>
#include <set>
#include <memory>

#include <PilsCore/types.h>

#include "gpudevice.h"

class GLFWwindow;

namespace pils
{
namespace gpu
{
    class Surface;
    class Swapchain;
}
}

namespace unisim
{

class Window;
class KeyboardEvent;
class MouseMoveEvent;
class MouseButtonEvent;
class MouseScrollEvent;


class WindowEventListener
{
public:
    WindowEventListener();
    virtual ~WindowEventListener();

    virtual void onWindowResize(const Window& window, int width, int height);
    virtual void onWindowKeyboard(const Window& window, const KeyboardEvent& event);
    virtual void onWindowMouseMove(const Window& window, const MouseMoveEvent& event);
    virtual void onWindowMouseButton(const Window& window, const MouseButtonEvent& event);
    virtual void onWindowMouseScroll(const Window& window, const MouseScrollEvent& event);
};


class WindowRegistry
{
public:
    static WindowRegistry& getInstance();

    void registerWindow(Window* window, uint64_t handle);
    void unregisterWindow(uint64_t handle);

    Window* getWindow(uint64_t handle);

private:
    std::map<uint64_t, Window*> _windows;
};

class WindowSurface;

class Window
{
public:
    Window(const GpuDevice& gpuDevice);
    Window(const GpuDevice& gpuDevice, int requestedWidth, int requestedHeight);
    ~Window();

    bool isValid() const;
    uint64_t handle() const;
    int width() const { return _width; }
    int height() const { return _height; }

    GLFWwindow* glfwWindow() const { return _glfwWindow; }

    const GpuDevice& gpuDevice() const { return _gpuDevice; }
    const pils::gpu::Surface& surface() const { return *_surface; }
    const pils::gpu::Swapchain& swapchain() const { return *_swapchain; }

    void registerEventListener(WindowEventListener* listener);
    void unregisterEventListener(WindowEventListener* listener);

    void onResize(int width, int height);
    void onKeyboard(const KeyboardEvent& event);
    void onMouseMove(const MouseMoveEvent& event);
    void onMouseButton(const MouseButtonEvent& event);
    void onMouseScroll(const MouseScrollEvent& event);

    bool shouldClose();
    void pollEvents();
    void present();
    void close();

private:
    bool InitGlfwNative();
    bool CreateGlfwSurface();

    const GpuDevice& _gpuDevice;

    GLFWwindow* _glfwWindow;
    std::set<WindowEventListener*> _eventListeners;
    std::unique_ptr<pils::gpu::Surface> _surface;
    std::unique_ptr<pils::gpu::Swapchain> _swapchain;

    int _width;
    int _height;
};

}

#endif // WINDOW_H
