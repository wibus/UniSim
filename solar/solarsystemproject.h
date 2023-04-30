#ifndef SOLAR_SYSTEM_PROJECT_H
#define SOLAR_SYSTEM_PROJECT_H

#include "../project.h"


namespace unisim
{

class Scene;
class CameraMan;


class SolarSystemProject : public Project
{
public:
    SolarSystemProject();

    int addView(Viewport viewport) override;
};

// IMPLEMENTATION //

}

#endif // SOLAR_SYSTEM_PROJECT_H
