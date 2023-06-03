#include "scene.h"

#include "object.h"
#include "material.h"
#include "body.h"


namespace unisim
{


DirectionalLight::DirectionalLight()
{

}

DirectionalLight::~DirectionalLight()
{

}

Sky::Sky()
{

}

void Sky::setTexture(const std::string& fileName)
{
    _texture.reset(Texture::load(fileName));
}


Scene::Scene() :
    _sky(new Sky())
{

}

Scene::~Scene()
{

}

}
