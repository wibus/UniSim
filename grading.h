#ifndef GRADING_H
#define GRADING_H

#include "graphic.h"

namespace unisim
{

class ColorGrading : public GraphicTask
{
public:
    ColorGrading();

    bool defineShaders(GraphicContext& context) override;
    bool defineResources(GraphicContext& context) override;

    void render(GraphicContext& context) override;

private:
    GLuint _vbo;
    GLuint _vao;
    GraphicProgramPtr _colorGradingProgram;
};

}

#endif // GRADING_H
