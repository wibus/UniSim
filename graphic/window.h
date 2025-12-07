#ifndef WINDOW_H
#define WINDOW_H

#include <map>
#include <set>

#include <PilsCore/types.h>


class GLFWwindow;


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


class Window
{
public:
    Window();
    Window(int requestedWidth, int requestedHeight);
    ~Window();

    bool isValid() const;
    uint64_t handle() const;
    int width() const { return _width; }
    int height() const { return _height; }

    void registerEventListener(WindowEventListener* listener);
    void unregisterEventListener(WindowEventListener* listener);

    void onResize(int width, int height);
    void onKeyboard(const KeyboardEvent& event);
    void onMouseMove(const MouseMoveEvent& event);
    void onMouseButton(const MouseButtonEvent& event);
    void onMouseScroll(const MouseScrollEvent& event);

    void ImGuiNewFrame();

    bool shouldClose();
    void pollEvents();
    void present();
    void close();

private:
    void ImGuiInitNative();
    void ImGuiNewFrameNative();

    GLFWwindow* _glfwWindow;
    std::set<WindowEventListener*> _eventListeners;

    int _width;
    int _height;
};

}

#endif // WINDOW_H
