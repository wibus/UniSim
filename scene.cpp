#include "scene.h"

#include "object.h"
#include "material.h"
#include "body.h"


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

Sky::Sky() :
    _quaternion(0, 0, 0, 1),
    _exposure(1)
{

}

void Sky::setTexture(const std::string& fileName)
{
    _texture.reset(Texture::load(fileName));
}


Scene::Scene(const std::string& name) :
    _name(name),
    _sky(new Sky())
{

}

Scene::~Scene()
{

}

}
