#ifndef PATH_TRACER_PROJECT_H
#define PATH_TRACER_PROJECT_H

#include "../engine/project.h"


namespace unisim
{

class Scene;
class CameraMan;


class PathTracerProject : public Project
{
public:
    PathTracerProject();

    int addView(Viewport viewport) override;
};

// IMPLEMENTATION //

}

#endif // PATH_TRACER_PROJECT_H
