#include "grading.h"

#include "../system/profiler.h"


namespace unisim
{

DefineProfilePointGpu(ColorGrading);

DeclareResource(PathTracerResult);
DeclareResource(FullScreenTriangle);


ColorGrading::ColorGrading() :
    GraphicTask("Color Grading")
{
    _colorGradingProgram = registerProgram("Color Grading");
}

bool ColorGrading::defineShaders(GraphicContext& context)
{
    if(!generateGraphicProgram(*_colorGradingProgram, "shaders/fullscreen.vert", "shaders/colorgrade.frag"))
        return false;

    return true;
}

bool ColorGrading::defineResources(GraphicContext& context)
{
    return true;
}

void ColorGrading::render(GraphicContext& context)
{
    ProfileGpu(ColorGrading);

    if(!_colorGradingProgram->isValid())
        return;

    GraphicProgramScope programScope(*_colorGradingProgram);

    GpuResourceManager& resources = context.resources;
    context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(PathTracerResult)), 0);
    context.device.draw(resources.get<GpuGeometryResource>(ResourceName(FullScreenTriangle)));
}

}
