#ifndef GRADINGTASK_H
#define GRADINGTASK_H

#include "../taskgraph/graphictask.h"

#include "../graphic/programinterface.h"
#include "../graphic/renderstates.h"


namespace unisim
{

class GradingTask : public GraphicTask
{
public:
    GradingTask();

    bool defineShaders(GraphicContext& context) override;
    
    void render(GraphicContext& context) override;

private:
    RenderStatesPtr _renderStates;
    ProgramPtr _colorGradingProgram;
    GpuProgramInterfacePtr _colorGradingPi;
};

}

#endif // GRADINGTASK_H
