#ifndef GRAVITYTASK_H
#define GRAVITYTASK_H

#include "../taskgraph/enginetask.h"


namespace unisim
{

class Scene;

class GravityTask : public EngineTask
{
public:
    GravityTask();

    void update(EngineContext& context) override;

public:
};

}

#endif // GRAVITYTASK_H
