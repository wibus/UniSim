#include "gradingtask.h"

#include "../system/profiler.h"

#include "../graphic/gpudevice.h"
#include "../graphic/renderstates.h"


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
    ProgramInterfaceTemplate colorGradingPiTemplate(ProgramType::Graphics, context.resources.framePiTemplate());
    colorGradingPiTemplate.declareTexture(0, {"Input"});

    _colorGradingPi.reset(new ProgramInterface(context.device, colorGradingPiTemplate));

    _renderStates.reset(new RenderStates(context.device));
    _renderStates->setView(context.view, true, false);
    _renderStates->setGeometry(&context.resources.get<GpuGeometryResource>(ResourceName(FullScreenTriangle)));

    _colorGradingProgram.reset();
    if(!context.shaders.generateGraphicProgram(
            context.device,
            colorGradingPiTemplate,
            *_renderStates,
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

    ProgramScope programScope(context.device, *_colorGradingProgram);

    const GpuGeometryResource& geometry = resources.get<GpuGeometryResource>(ResourceName(FullScreenTriangle));

    _renderStates->setView(context.view, true, false);
    _renderStates->setGeometry(&geometry);

    RenderPassScope renderPassScope(context.device, *_renderStates);

    context.device.draw(geometry);
}

}
