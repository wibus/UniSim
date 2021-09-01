#ifndef GRAVITY_H
#define GRAVITY_H

#include <vector>

#include "body.h"


namespace unisim
{

class Gravity
{
public:
    Gravity();

    bool initialize(const std::vector<Body *> &bodies) const;

    void update(std::vector<Body*>& bodies, double dt);

public:
};

}

#endif // GRAVITY_H
