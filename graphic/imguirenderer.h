#ifndef IMGUIRENDERER_H
#define IMGUIRENDERER_H

#include <memory>

#include <PilsCore/types.h>


namespace unisim
{

class Window;
class View;
class GpuDevice;


class ImGuiRenderer
{
public:
    ImGuiRenderer(const View& view);
    ~ImGuiRenderer();

    void ImGuiNewFrame() const;

    void render(GpuDevice& gpuDevice, const View& view) const;

private:
    void ImGuiInitNative();
    void ImGuiNewFrameNative() const;
    void renderNative(GpuDevice& gpuDevice, const View& view) const;

    const View& _view;

    std::unique_ptr<class ImGuiNativeData> _imguiNativeData;
};

}

#endif // IMGUIRENDERER_H
