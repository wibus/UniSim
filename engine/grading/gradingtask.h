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
    GraphicProgramPtr _colorGradingProgram;
    GpuProgramInterfacePtr _colorGradingGpi;
};

}

#endif // GRADINGTASK_H
