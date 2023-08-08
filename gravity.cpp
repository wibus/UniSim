#include "gravity.h"

#include "units.h"
#include "scene.h"
#include "object.h"
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

    std::vector<std::shared_ptr<Object>>& objects = context.scene.objects();

    if(objects.empty())
        return;

    for(std::size_t i = 0; i < objects.size()-1; ++i)
    {
        if(objects[i]->body().get() == nullptr)
            continue;

        Body& bodyA = *objects[i]->body();

        for(std::size_t j = i+1; j < objects.size(); ++j)
        {
            if(objects[j]->body().get() == nullptr)
                continue;

            Body& bodyB = *objects[j]->body();

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

    for(std::size_t i = 0; i < objects.size(); ++i)
    {
        if(objects[i]->body().get() == nullptr)
            continue;

        Body& body = *objects[i]->body();

        if(body.isStatic())
            continue;

        body.setQuaternion(quatMul(body.quaternion(), quat(glm::dvec3(0, 0, 1), body.angularSpeed() * context.dt)));
        body.setPosition(body.position() + context.dt * body.linearVelocity());
    }
}

}
