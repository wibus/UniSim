#include "pathtracerproject.h"

#include "pathtracercamera.h"
#include "pathtracerscene.h"


namespace unisim
{

PathTracerProject::PathTracerProject()
{
    setScene(new PathTracerScene());
}

int PathTracerProject::addView(const std::shared_ptr<View>& view)
{
    return addCameraMan(new PathTracerCameraMan(scene(), *view));
}

}
