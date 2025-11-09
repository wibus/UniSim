#include "view.h"

#include <PilsCore/Utils/Assert.h>

#include "graphic_gl.h"

#include "window.h"


namespace unisim
{

ViewEventListener::ViewEventListener()
{
}

ViewEventListener::~ViewEventListener()
{
}

void ViewEventListener::onViewportChanged(const View& view, const Viewport& viewport)
{

}


View::View(Window& window) :
    _window(window),
    _viewport{window.width(), window.height()}
{
    _window.registerEventListener(this);
    glViewport(0, 0, _viewport.width, _viewport.height);
}

View::~View()
{
    _window.unregisterEventListener(this);
    PILS_ASSERT(_eventListeners.empty(), "View is being destroyed with active listeners");
}

void View::onWindowResize(const Window& window, int width, int height)
{
    _viewport.width = width;
    _viewport.height = height;

    glViewport(0, 0, _viewport.width, _viewport.height);

    for(auto listener : _eventListeners)
        listener->onViewportChanged(*this, _viewport);
}

void View::registerEventListener(ViewEventListener* listener)
{
    if (listener != nullptr)
        _eventListeners.insert(listener);
}

void View::unregisterEventListener(ViewEventListener* listener)
{
    if (listener != nullptr)
        _eventListeners.erase(listener);
}

}
