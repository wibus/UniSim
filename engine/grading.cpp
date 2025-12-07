#include "grading.h"

#include "../system/profiler.h"


namespace unisim
{

DefineProfilePointGpu(ColorGrading);

DeclareResource(PathTracerResult);


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
    float points[] = {
      -1.0f, -1.0f,  0.0f,
       3.0f, -1.0f,  0.0f,
      -1.0f,  3.0f,  0.0f,
    };

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), points, GL_STATIC_DRAW);

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    return true;
}

void ColorGrading::render(GraphicContext& context)
{
    ProfileGpu(ColorGrading);

    if(!_colorGradingProgram->isValid())
        return;
    
    GpuResourceManager& resources = context.resources;

    const auto& pathTracerResult = resources.get<GpuImageResource>(ResourceName(PathTracerResult));

    GraphicProgramScope programScope(*_colorGradingProgram);

    context.device.bindTexture(pathTracerResult, 0);

    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
}

}
