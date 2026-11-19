#ifndef VIEW_H
#define VIEW_H

#include <set>
#include <vector>
#include <memory>

#include "window.h"

#ifdef UNISIM_GRAPHIC_BACKEND_VK
namespace pils
{
namespace gpu
{
class RenderPass;
class FrameBuffer;
}
}
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

    const Window& window() const { return _window; }
    const Viewport& viewport() const { return _viewport; }

    virtual void onWindowResizeBefore(const Window& window, int width, int height) override;
    virtual void onWindowResizeAfter(const Window& window, int width, int height) override;

    void registerEventListener(ViewEventListener* listener);
    void unregisterEventListener(ViewEventListener* listener);

    void setViewport() const;

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    const pils::gpu::RenderPass& renderPass() const {  return *_renderPass; }
    const pils::gpu::FrameBuffer& currentFrameBuffer() const;
    const pils::gpu::FrameBuffer& frameBufferPrototype() const;
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
    void resetViewportNative();
    void resizeViewportNative();
    void setViewportNative() const;

    Window& _window;
    Viewport _viewport;
    std::set<ViewEventListener*> _eventListeners;

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::unique_ptr<pils::gpu::RenderPass> _renderPass;
    std::vector<std::unique_ptr<pils::gpu::FrameBuffer>> _frameBuffers;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

}

#endif // VIEW_H
