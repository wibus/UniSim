#ifndef GRADING_H
#define GRADING_H

#include "taskgraph.h"

namespace unisim
{

class ColorGrading : public GraphicTask
{
public:
    ColorGrading();
    
    bool defineShaders(Context& context) override;
    bool defineResources(Context& context) override;
    
    void render(Context& context) override;

private:
    GLuint _vbo;
    GLuint _vao;
    GraphicProgramPtr _colorGradingProgram;
};

}

#endif // GRADING_H
