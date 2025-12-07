#ifndef VIEW_H
#define VIEW_H

#include <set>

#include "window.h"

#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include "view_gl.h"
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include "view_vk.h"
#endif // UNISIM_GRAPHIC_BACKEND_VK

namespace unisim
{

class View;


struct Viewport
{
    int width;
    int height;

    bool operator==(const Viewport& viewport) const
    {
        return viewport.width == width && viewport.height == height;
    }

    bool operator!=(const Viewport& viewport) const
    {
        return !(viewport == *this);
    }
};


class ViewEventListener
{
public:
    ViewEventListener();
    virtual ~ViewEventListener();

    virtual void onViewportChanged(const View& view, const Viewport& viewport);
};


class View : public WindowEventListener
{
public:
    View(Window& window);
    virtual ~View();

    const Viewport& viewport() const { return _viewport; }

    void onWindowResize(const Window& window, int width, int height) override;

    void registerEventListener(ViewEventListener* listener);
    void unregisterEventListener(ViewEventListener* listener);

    void setViewport() const;

private:
    void setViewportNative() const;

    Window& _window;
    Viewport _viewport;
    std::set<ViewEventListener*> _eventListeners;
};

}

#endif // VIEW_H
