#ifndef GRADINGTASK_H
#define GRADINGTASK_H

#include "../taskgraph/graphictask.h"


namespace unisim
{

class GradingTask : public GraphicTask
{
public:
    GradingTask();

    bool defineShaders(GraphicContext& context) override;
    
    void render(GraphicContext& context) override;

private:
    ProgramPtr _colorGradingProgram;
    GpuProgramInterfacePtr _colorGradingPi;
};

}

#endif // GRADINGTASK_H
