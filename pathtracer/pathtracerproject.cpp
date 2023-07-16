#include "pathtracerproject.h"

#include "pathtracercamera.h"
#include "pathtracerscene.h"
#include "pathtracerui.h"


namespace unisim
{

PathTracerProject::PathTracerProject()
{
    reset(new PathTracerScene(), new PathTracerUi());
}

int PathTracerProject::addView(Viewport viewport)
{
    int cameraManId = addCameraMan(new PathTracerCameraMan(scene(), viewport));
    scene().initializeCamera(cameraMan(cameraManId).camera());
    return cameraManId;
}

}
