#include "solarsystemproject.h"

#include "solarsystemscene.h"
#include "solarsystemcamera.h"


namespace unisim
{

SolarSystemProject::SolarSystemProject()
{
    reset(new SolarSystemScene());
}

int SolarSystemProject::addView(Viewport viewport)
{
    return addCameraMan(new SolarSystemCameraMan(scene(), viewport));
}

}
