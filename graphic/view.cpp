#include "view.h"

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Pass.h>

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
    resizeViewportNative();
    setViewport();
}

View::~View()
{
    _window.unregisterEventListener(this);
    PILS_ASSERT(_eventListeners.empty(), "View is being destroyed with active listeners");

    _frameBuffers.clear();
    _renderPass.reset();
}

void View::onWindowResizeBefore(const Window& window, int width, int height)
{
    resetViewportNative();
}

void View::onWindowResizeAfter(const Window& window, int width, int height)
{
    _viewport.width = width;
    _viewport.height = height;

    resizeViewportNative();
    setViewport();

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

void View::setViewport() const
{
    setViewportNative();
}

const pils::gpu::FrameBuffer& View::frameBuffer() const
{
    PILS_ASSERT(_window.imageIndex() >= 0, "Swapchain image index is undefined");
    PILS_ASSERT(_window.imageIndex() < _frameBuffers.size(), "No framebuffer defined for this image index");
    return *_frameBuffers[_window.imageIndex()];
}

}
