#include "project.h"

#include "scene.h"
#include "ui.h"


namespace unisim
{

Project::Project()
{

}

Project::~Project()
{

}

void Project::reset(Scene* scene, Ui* ui)
{
    _scene.reset(scene);

    auto oldCameraMen = _cameraMen;
    _cameraMen.clear();

    for(const auto& cameraMan : oldCameraMen)
    {
        addView(cameraMan->camera().viewport());
    }

    _ui.reset(ui);
}

int Project::addCameraMan(CameraMan* cameraMan)
{
    int index = _cameraMen.size();
    _cameraMen.emplace_back(cameraMan);
    return index;
}

}
