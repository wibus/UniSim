#include "primitive.h"


namespace unisim
{

const char* Primitive::Type_Names[Primitive::Type_Count] = {
    "Mesh",
    "Sphere",
    "Plane"
};

Primitive::Primitive(Type type) :
    _type(type)
{

}

Primitive::~Primitive()
{

}


Mesh::Mesh() :
    Primitive(Primitive::Mesh)
{

}

bool Mesh::load(const std::string& fileName)
{
    return false;
}


Sphere::Sphere() :
    Primitive(Primitive::Sphere),
    _radius(1.0f)
{

}

Sphere::Sphere(float radius) :
    Primitive(Primitive::Sphere),
    _radius(radius)
{

}

Plane::Plane() :
    Primitive(Primitive::Plane),
    _scale(1)
{

}

Plane::Plane(float scale) :
    Primitive(Primitive::Plane),
    _scale(scale)
{

}

}
