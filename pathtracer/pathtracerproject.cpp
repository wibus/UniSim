#include "pathtracerproject.h"

#include "pathtracerscene.h"
#include "pathtracercamera.h"


namespace unisim
{

PathTracerProject::PathTracerProject()
{
    reset(new PathTracerScene());
}

int PathTracerProject::addView(Viewport viewport)
{
    return addCameraMan(new PathTracerCameraMan(scene(), viewport));
}

}
