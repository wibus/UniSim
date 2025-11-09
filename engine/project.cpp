#include "project.h"

#include "scene.h"


namespace unisim
{

Project::Project()
{

}

Project::~Project()
{

}

void Project::setScene(Scene* scene)
{
    _scene.reset(scene);
}

int Project::addCameraMan(CameraMan* cameraMan)
{
    int index = _cameraMen.size();
    _cameraMen.emplace_back(cameraMan);
    return index;
}

}
