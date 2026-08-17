#ifndef VIEW_H
#define VIEW_H

#include <set>
#include <memory>

#include "window.h"

#ifdef UNISIM_GRAPHIC_BACKEND_GL
#include "view_gl.h"
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
#include "view_vk.h"
#endif // UNISIM_GRAPHIC_BACKEND_VK


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

    void onWindowResize(const Window& window, int width, int height) override;

    void registerEventListener(ViewEventListener* listener);
    void unregisterEventListener(ViewEventListener* listener);

    void setViewport() const;

    const pils::gpu::RenderPass& renderPass() const {  return *_renderPass; }
    const pils::gpu::FrameBuffer& frameBuffer() const { return *_frameBuffer; }

private:
    void resizeViewportNative();
    void setViewportNative() const;

    Window& _window;
    Viewport _viewport;
    std::set<ViewEventListener*> _eventListeners;

    std::unique_ptr<pils::gpu::RenderPass> _renderPass;
    std::unique_ptr<pils::gpu::FrameBuffer> _frameBuffer;
};

}

#endif // VIEW_H
