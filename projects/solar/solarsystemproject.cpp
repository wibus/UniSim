#include "solarsystemproject.h"

#include "solarsystemcamera.h"
#include "solarsystemscene.h"


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
