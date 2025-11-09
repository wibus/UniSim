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

    int addView(const std::shared_ptr<View>& view) override;
};

// IMPLEMENTATION //

}

#endif // PATH_TRACER_PROJECT_H
