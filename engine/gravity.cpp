#include "gravity.h"

#include "units.h"
#include "scene.h"
#include "instance.h"
#include "body.h"
#include "profiler.h"


namespace unisim
{

DefineProfilePoint(Gravity);


Gravity::Gravity() :
    EngineTask("Gravity")
{

}

void Gravity::update(EngineContext& context)
{
    Profile(Gravity);

    std::vector<std::shared_ptr<Instance>>& instances = context.scene.instances();

    if(instances.empty())
        return;

    for(std::size_t i = 0; i < instances.size()-1; ++i)
    {
        if(instances[i]->body().get() == nullptr)
            continue;

        Body& bodyA = *instances[i]->body();

        for(std::size_t j = i+1; j < instances.size(); ++j)
        {
            if(instances[j]->body().get() == nullptr)
                continue;

            Body& bodyB = *instances[j]->body();

            glm::dvec3 dist = bodyB.position() - bodyA.position();
            double dSquared = glm::dot(dist, dist);
            glm::dvec3 U = glm::normalize(dist);

            double F = G * bodyA.mass() * bodyB.mass() / dSquared;

            if(!bodyA.isStatic())
                bodyA.setLinearVelocity(bodyA.linearVelocity() + context.dt * U * (F / bodyA.mass()));

            if(!bodyB.isStatic())
                bodyB.setLinearVelocity(bodyB.linearVelocity() - context.dt * U * (F / bodyB.mass()));
        }
    }

    for(std::size_t i = 0; i < instances.size(); ++i)
    {
        if(instances[i]->body().get() == nullptr)
            continue;

        Body& body = *instances[i]->body();

        if(body.isStatic())
            continue;

        body.setQuaternion(quatMul(body.quaternion(), quat(glm::dvec3(0, 0, 1), body.angularSpeed() * context.dt)));
        body.setPosition(body.position() + context.dt * body.linearVelocity());
    }
}

}
