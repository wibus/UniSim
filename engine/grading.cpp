#include "grading.h"

#include "../system/profiler.h"

#include "../graphic/gpudevice.h"


namespace unisim
{

DefineProfilePointGpu(ColorGrading);

DeclareResource(PathTracerResult);
DeclareResource(FullScreenTriangle);


ColorGrading::ColorGrading() :
    GraphicTask("Color Grading")
{
}

bool ColorGrading::defineShaders(GraphicContext& context)
{
    _colorGradingGpi.reset(new GpuProgramInterface());
    _colorGradingGpi->declareTexture({"Input"});

    _colorGradingProgram.reset();
    if(!generateGraphicProgram(_colorGradingProgram, "Color Grading", "shaders/fullscreen.vert", "shaders/colorgrade.frag"))
        return false;

    return true;
}

void ColorGrading::render(GraphicContext& context)
{
    ProfileGpu(ColorGrading);

    if(!_colorGradingProgram->isValid())
        return;

    CompiledGpuProgramInterface compiledGpi;
    if (!_colorGradingGpi->compile(compiledGpi, *_colorGradingProgram))
        return;

    GraphicProgramScope programScope(*_colorGradingProgram);

    GpuResourceManager& resources = context.resources;
    context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(PathTracerResult)),
                               compiledGpi.getTextureBindPoint("Input"));

    context.device.draw(resources.get<GpuGeometryResource>(ResourceName(FullScreenTriangle)));
}

}
