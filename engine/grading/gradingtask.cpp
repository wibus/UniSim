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
    ProgramInterfaceTemplate colorGradingPiTemplate(context.resources.framePiTemplate());
    colorGradingPiTemplate.declareTexture({"Input"});

    _colorGradingPi.reset(new ProgramInterface(context.device, colorGradingPiTemplate));

    _colorGradingProgram.reset();
    if(!context.shaders.generateGraphicProgram(
            context.device,
            colorGradingPiTemplate,
            _colorGradingProgram,
            "Color Grading",
            "shaders/fullscreen.vert",
            "shaders/colorgrade.frag"))
        return false;

    return true;
}

void GradingTask::render(GraphicContext& context)
{
    ProfileGpu(ColorGrading);

    if(!_colorGradingProgram->isValid())
        return;

    GpuResourceManager& resources = context.resources;
    _colorGradingPi->setTexture("Input", resources.get<GpuImageResource>(ResourceName(PathTracerResult)));

    if (!_colorGradingPi->compileAndSet(context.device, *_colorGradingProgram))
        return;

    ProgramScope programScope(*_colorGradingProgram);

    context.device.draw(resources.get<GpuGeometryResource>(ResourceName(FullScreenTriangle)));
}

}
