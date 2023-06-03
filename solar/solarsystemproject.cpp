#include "solarsystemproject.h"

#include "solarsystemcamera.h"
#include "solarsystemscene.h"
#include "solarsystemui.h"


namespace unisim
{

SolarSystemProject::SolarSystemProject()
{
    reset(new SolarSystemScene(), new SolarSystemUi());
}

int SolarSystemProject::addView(Viewport viewport)
{
    return addCameraMan(new SolarSystemCameraMan(scene(), viewport));
}

}
