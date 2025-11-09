#ifndef VIEW_H
#define VIEW_H

#include <set>

#include "window.h"


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

private:
    Window& _window;
    Viewport _viewport;
    std::set<ViewEventListener*> _eventListeners;
};

}

#endif // VIEW_H
