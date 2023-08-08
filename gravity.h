#ifndef GRAVITY_H
#define GRAVITY_H

#include <vector>
#include <memory>

#include "engine.h"


namespace unisim
{

class Scene;

class Gravity : public EngineTask
{
public:
    Gravity();

    void update(EngineContext& context) override;

public:
};

}

#endif // GRAVITY_H
