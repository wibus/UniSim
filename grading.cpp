#include "grading.h"

#include "camera.h"
#include "profiler.h"


namespace unisim
{

DefineProfilePointGpu(Clear);
DefineProfilePointGpu(ColorGrade);

DeclareResource(PathTracerResult);


ColorGrading::ColorGrading() :
    GraphicTask("Color Grading")
{

}

bool ColorGrading::defineResources(GraphicContext& context)
{
    if(!generateGraphicProgram(_colorGradingId, "shaders/fullscreen.vert", "shaders/colorgrade.frag"))
        return false;

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
    ResourceManager& resources = context.resources;

    GLuint pathTraceTexId = resources.get<GpuImageResource>(ResourceName(PathTracerResult)).texId;

    {
        ProfileGpu(Clear);

        const Viewport& viewport = context.camera.viewport();
        glViewport(0, 0, viewport.width, viewport.height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    {
        ProfileGpu(ColorGrade);

        glUseProgram(_colorGradingId);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pathTraceTexId);

        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
    }

    glUseProgram(0);
}

}
