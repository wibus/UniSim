#ifndef PATH_TRACER_SCENE_H
#define PATH_TRACER_SCENE_H

#include "../scene.h"


namespace unisim
{

class PathTracerScene : public Scene
{
public:
    PathTracerScene();

    void initializeCamera(Camera& camera) override;
};


// IMPLEMENTATION //

}

#endif // PATH_TRACER_SCENE_H
