#ifndef GRAVITY_H
#define GRAVITY_H

#include <vector>
#include <memory>


namespace unisim
{

class Scene;

class Gravity
{
public:
    Gravity();

    bool initialize(const Scene& scene) const;

    void update(Scene& scene, double dt);

public:
};

}

#endif // GRAVITY_H
