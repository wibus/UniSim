#include "scene.h"

#include "object.h"
#include "material.h"
#include "body.h"
#include "sky.h"


namespace unisim
{


Scene::Scene(const std::string& name) :
    _name(name),
    _sky(new SkySphere())
{

}

Scene::~Scene()
{

}

void Scene::initializeCamera(Camera& camera)
{

}

}
