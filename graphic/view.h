#ifndef VIEW_H
#define VIEW_H

#include <set>
#include <vector>
#include <memory>

#include "window.h"


namespace pils
{
namespace gpu
{
class RenderPass;
class FrameBuffer;
}
}


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

    const pils::gpu::RenderPass& renderPass() const {  return *_renderPass; }
    const pils::gpu::FrameBuffer& frameBuffer() const;

private:
    void resetViewportNative();
    void resizeViewportNative();
    void setViewportNative() const;

    Window& _window;
    Viewport _viewport;
    std::set<ViewEventListener*> _eventListeners;

    std::unique_ptr<pils::gpu::RenderPass> _renderPass;
    std::vector<std::unique_ptr<pils::gpu::FrameBuffer>> _frameBuffers;
};

}

#endif // VIEW_H
