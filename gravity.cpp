#include "gravity.h"

#include "units.h"
#include "body.h"


namespace unisim
{

Gravity::Gravity()
{

}

bool Gravity::initialize(const std::vector<std::shared_ptr<Object>>& objects) const
{
    return true;
}

void Gravity::update(std::vector<std::shared_ptr<Object>>& objects, double dt)
{
    if(objects.empty())
        return;

    for(std::size_t i = 0; i < objects.size()-1; ++i)
    {
        Body& bodyA = *objects[i]->body();

        for(std::size_t j = i+1; j < objects.size(); ++j)
        {
            Body& bodyB = *objects[j]->body();

            glm::dvec3 dist = bodyB.position() - bodyA.position();
            double dSquared = glm::dot(dist, dist);
            glm::dvec3 U = glm::normalize(dist);

            double F = G * bodyA.mass() * bodyB.mass() / dSquared;

            if(!bodyA.isStatic())
                bodyA.setLinearMomentum(bodyA.linearMomentum() + dt * U * (F / bodyA.mass()));

            if(!bodyB.isStatic())
                bodyB.setLinearMomentum(bodyB.linearMomentum() - dt * U * (F / bodyB.mass()));
        }
    }

    for(std::size_t i = 0; i < objects.size(); ++i)
    {
        Body& body = *objects[i]->body();

        if(body.isStatic())
            continue;

        body.setQuaternion(quatMul(body.quaternion(), quat(glm::dvec3(0, 0, 1), body.rotation() * dt)));
        body.setPosition(body.position() + dt * body.linearMomentum());
    }
}

}
