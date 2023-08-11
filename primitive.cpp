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

Mesh Mesh::cube(float length, float uvScale)
{
    Mesh mesh;

    //                       {position}, {  normal  }, { uv }

    // X+
    mesh._vertices.push_back({{1, 0, 0}, { 1,  0,  0}, {0, 0}});
    mesh._vertices.push_back({{1, 1, 0}, { 1,  0,  0}, {1, 0}});
    mesh._vertices.push_back({{1, 1, 1}, { 1,  0,  0}, {1, 1}});
    mesh._vertices.push_back({{1, 0, 1}, { 1,  0,  0}, {0, 1}});

    mesh._triangles.push_back({0, 1, 2});
    mesh._triangles.push_back({0, 2, 3});

    // X-
    mesh._vertices.push_back({{0, 1, 0}, {-1,  0,  0}, {0, 0}});
    mesh._vertices.push_back({{0, 0, 0}, {-1,  0,  0}, {1, 0}});
    mesh._vertices.push_back({{0, 0, 1}, {-1,  0,  0}, {1, 1}});
    mesh._vertices.push_back({{0, 1, 1}, {-1,  0,  0}, {0, 1}});

    mesh._triangles.push_back({4, 5, 6});
    mesh._triangles.push_back({4, 6, 7});


    // Y+
    mesh._vertices.push_back({{1, 1, 0}, { 0,  1,  0}, {0, 0}});
    mesh._vertices.push_back({{0, 1, 0}, { 0,  1,  0}, {1, 0}});
    mesh._vertices.push_back({{0, 1, 1}, { 0,  1,  0}, {1, 1}});
    mesh._vertices.push_back({{1, 1, 1}, { 0,  1,  0}, {0, 1}});

    mesh._triangles.push_back({8, 9, 10});
    mesh._triangles.push_back({8, 10, 11});

    // Y-
    mesh._vertices.push_back({{0, 0, 0}, { 0,  -1,  0}, {0, 0}});
    mesh._vertices.push_back({{1, 0, 0}, { 0,  -1,  0}, {1, 0}});
    mesh._vertices.push_back({{1, 0, 1}, { 0,  -1,  0}, {1, 1}});
    mesh._vertices.push_back({{0, 0, 1}, { 0,  -1,  0}, {0, 1}});

    mesh._triangles.push_back({12, 13, 14});
    mesh._triangles.push_back({12, 14, 15});


    // Z+
    mesh._vertices.push_back({{0, 0, 1}, { 0,  0,  1}, {0, 0}});
    mesh._vertices.push_back({{1, 0, 1}, { 0,  0,  1}, {1, 0}});
    mesh._vertices.push_back({{1, 1, 1}, { 0,  0,  1}, {1, 1}});
    mesh._vertices.push_back({{0, 1, 1}, { 0,  0,  1}, {0, 1}});

    mesh._triangles.push_back({16, 17, 18});
    mesh._triangles.push_back({16, 18, 19});

    // Z-
    mesh._vertices.push_back({{0, 0, 0}, { 0,  0, -1}, {0, 0}});
    mesh._vertices.push_back({{1, 1, 0}, { 0,  0, -1}, {1, 0}});
    mesh._vertices.push_back({{1, 1, 0}, { 0,  0, -1}, {1, 1}});
    mesh._vertices.push_back({{1, 0, 0}, { 0,  0, -1}, {0, 1}});

    mesh._triangles.push_back({20, 21, 22});
    mesh._triangles.push_back({20, 22, 23});


    // Center cube on origin and scale
    for(auto& vert : mesh._vertices)
    {
        vert.position -= glm::vec3(0.5, 0.5, 0.5);
        vert.position *= length;
        vert.uv *= uvScale;
    }

    return mesh;
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
