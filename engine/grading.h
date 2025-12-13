#ifndef GRADING_H
#define GRADING_H

#include "graphictask.h"

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
    GraphicProgramPtr _colorGradingProgram;
    GpuProgramInterfacePtr _colorGradingGpi;
};

}

#endif // GRADING_H
