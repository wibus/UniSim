#include "solarsystemproject.h"

#include "solarsystemcamera.h"
#include "solarsystemscene.h"


namespace unisim
{

SolarSystemProject::SolarSystemProject()
{
    setScene(new SolarSystemScene());
}

int SolarSystemProject::addView(const std::shared_ptr<View>& view)
{
    return addCameraMan(new SolarSystemCameraMan(scene(), *view));
}

}
