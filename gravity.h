#ifndef GRAVITY_H
#define GRAVITY_H

#include <vector>
#include <memory>

#include "object.h"


namespace unisim
{

class Gravity
{
public:
    Gravity();

    bool initialize(const std::vector<std::shared_ptr<Object>>& objects) const;

    void update(std::vector<std::shared_ptr<Object>>& objects, double dt);

public:
};

}

#endif // GRAVITY_H
