#ifndef WINDOW_H
#define WINDOW_H

#include <map>
#include <set>
#include <memory>
#include <vector>

#include <PilsCore/types.h>

#include "gpudevice.h"

class GLFWwindow;

namespace pils
{
namespace gpu
{
    class Fence;
    class Surface;
    class Swapchain;
    class CommandList;
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

    virtual void onWindowResizeBefore(const Window& window, int width, int height);
    virtual void onWindowResizeAfter(const Window& window, int width, int height);
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
    Window(GpuDevice& gpuDevice);
    Window(GpuDevice& gpuDevice, int requestedWidth, int requestedHeight);
    ~Window();

    bool isValid() const;
    uint64_t handle() const;
    int width() const { return _width; }
    int height() const { return _height; }

    GLFWwindow* glfwWindow() const { return _glfwWindow; }

    const GpuDevice& gpuDevice() const { return _gpuDevice; }
    const pils::gpu::Surface& surface() const { return *_surface; }
    const pils::gpu::Swapchain& swapchain() const { return *_swapchain; }
    uint32_t imageIndex() const;

    void registerEventListener(WindowEventListener* listener);
    void unregisterEventListener(WindowEventListener* listener);

    void onResize(int width, int height);
    void onKeyboard(const KeyboardEvent& event);
    void onMouseMove(const MouseMoveEvent& event);
    void onMouseButton(const MouseButtonEvent& event);
    void onMouseScroll(const MouseScrollEvent& event);

    bool shouldClose();
    void pollEvents();
    bool newFrame();
    bool present();
    void close();

private:
    bool InitGlfwNative();
    bool CreateGlfwSurface();
    void resetViewportNative();
    void resizeViewportNative();
    bool newFrameNative();
    bool presentNative();
    void closeNative();

    GpuDevice& _gpuDevice;

    GLFWwindow* _glfwWindow;
    std::set<WindowEventListener*> _eventListeners;
    std::unique_ptr<pils::gpu::Surface> _surface;
    std::unique_ptr<pils::gpu::Swapchain> _swapchain;

    std::vector<std::unique_ptr<pils::gpu::Fence>> _frameFences;
    std::vector<std::shared_ptr<pils::gpu::CommandList>> _frameCommandLists;

    int _width;
    int _height;
};

}

#endif // WINDOW_H
