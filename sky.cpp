#include "sky.h"

#include "material.h"


namespace unisim
{

DefineResource(SkyMap);

Sky::Sky(Mapping mapping) :
    _mapping(mapping),
    _quaternion(0, 0, 0, 1),
    _exposure(1)
{

}

Sky::~Sky()
{

}

SkySphere::SkySphere() :
    Sky(Mapping::Sphere)
{
    _texture.reset(new Texture(Texture::BLACK_Float32));
}

SkySphere::SkySphere(const std::string& fileName) :
    Sky(Mapping::Sphere)
{
    _texture.reset(Texture::load(fileName));
}

unsigned int SkySphere::width() const
{
    if(_texture)
        return _texture->width;
    else
        return 0;
}

unsigned int SkySphere::height() const
{
    if(_texture)
        return _texture->height;
    else
        return 0;
}

std::shared_ptr<GraphicTask> SkySphere::createGraphicTask()
{
    return std::make_shared<SkySphere::Task>(_texture);
}

SkySphere::Task::Task(const std::shared_ptr<Texture>& texture) :
    GraphicTask("SkySphere"),
    _texture(texture)
{

}

bool SkySphere::Task::defineResources(GraphicContext& context)
{
    ResourceManager& resources = context.resources;
    resources.define<GpuTextureResource>(ResourceName(SkyMap), {*_texture});
    return true;
}

}
