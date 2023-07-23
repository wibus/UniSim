#include "mesh.h"

namespace unisim
{

const char* PrimitiveType_Names[PrimitiveType::Count] = {
    "Sphere",
    "Terrain"
};

Mesh::Mesh(PrimitiveType primitiveType, float radius) :
    _primitiveType(primitiveType),
    _radius(radius)
{

}

Mesh::~Mesh()
{

}

}
