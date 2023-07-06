#include "scene.h"

#include "object.h"
#include "material.h"
#include "body.h"
#include "sky.h"


namespace unisim
{


DirectionalLight::DirectionalLight(const std::string& name) :
    _name(name),
    _emissionColor(1, 1, 1),
    _emissionLuminance(1)
{

}

DirectionalLight::~DirectionalLight()
{

}

Scene::Scene(const std::string& name) :
    _name(name),
    _sky(new SkySphere())
{

}

Scene::~Scene()
{

}

}
