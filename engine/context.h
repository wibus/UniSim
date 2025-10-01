#ifndef CONTEXT_H
#define CONTEXT_H

#include "resource.h"

namespace unisim
{

class Scene;
class Camera;


struct GraphicSettings
{
    bool unbiased;
};

struct Context
{
    const Scene& scene;
    const Camera& camera;
    ResourceManager& resources;
    const GraphicSettings& settings;
};

}

#endif // CONTEXT_H
