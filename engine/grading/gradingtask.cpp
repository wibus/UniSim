#include "gradingtask.h"

#include "../system/profiler.h"

#include "../graphic/gpudevice.h"


namespace unisim
{

DefineProfilePointGpu(ColorGrading);

DeclareResource(PathTracerResult);
DeclareResource(FullScreenTriangle);


GradingTask::GradingTask() :
    GraphicTask("Color Grading")
{
}

bool GradingTask::defineShaders(GraphicContext& context)
{
    GpuProgramInterfaceTemplate colorGradingTemplate(context.resources.frameGpiTemplate());
    colorGradingTemplate.declareTexture({"Input"});

    _colorGradingGpi.reset(new GpuProgramInterface(context.device, colorGradingTemplate));

    _colorGradingProgram.reset();
    if(!generateGraphicProgram(context.device, colorGradingTemplate, _colorGradingProgram, "Color Grading", "shaders/fullscreen.vert", "shaders/colorgrade.frag"))
        return false;

    return true;
}

void GradingTask::render(GraphicContext& context)
{
    ProfileGpu(ColorGrading);

    if(!_colorGradingProgram->isValid())
        return;

    GpuResourceManager& resources = context.resources;
    _colorGradingGpi->setTexture("Input", resources.get<GpuImageResource>(ResourceName(PathTracerResult)));

    if (!_colorGradingGpi->compileAndSet(context.device, *_colorGradingProgram))
        return;

    GraphicProgramScope programScope(*_colorGradingProgram);

    context.device.draw(resources.get<GpuGeometryResource>(ResourceName(FullScreenTriangle)));
}

}
