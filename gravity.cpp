#include "gravity.h"

#include "units.h"
#include "body.h"


namespace unisim
{

Gravity::Gravity()
{

}

bool Gravity::initialize(const std::vector<Body*>& bodies) const
{
    return true;
}

void Gravity::update(std::vector<Body*>& bodies, double dt)
{
    for(std::size_t i = 0; i < bodies.size()-1; ++i)
    {
        Body& bodyA = *bodies[i];

        for(std::size_t j = i+1; j < bodies.size(); ++j)
        {
            Body& bodyB = *bodies[j];

            glm::dvec3 dist = bodyB.position() - bodyA.position();
            double dSquared = glm::dot(dist, dist);
            glm::dvec3 U = glm::normalize(dist);

            double F = G * bodyA.mass() * bodyB.mass() / dSquared;

            bodyA.setLinearMomentum(bodyA.linearMomentum() + dt * U * (F / bodyA.mass()));
            bodyB.setLinearMomentum(bodyB.linearMomentum() - dt * U * (F / bodyB.mass()));
        }
    }

    for(std::size_t i = 0; i < bodies.size(); ++i)
    {
        Body& body = *bodies[i];

        body.setQuaternion(quatMul(body.quaternion(), quat(glm::dvec3(0, 0, 1), body.rotation() * dt)));
        body.setPosition(body.position() + dt * body.linearMomentum());
    }
}

}
