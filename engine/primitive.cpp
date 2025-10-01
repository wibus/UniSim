#include "primitive.h"

#include <imgui/imgui.h>

#include "material.h"


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

void Primitive::ui()
{
    if(ImGui::TreeNode("Material"))
    {
        if(_material)
            _material->ui();
        else
            ImGui::Text("No Material");

        ImGui::TreePop();
    }
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

void Mesh::ui()
{
    Primitive::ui();

    ImGui::Text("Vertex count: %u", (uint)_vertices.size());
    ImGui::Text("Triangle count: %u", (uint)_triangles.size());
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

void Sphere::ui()
{
    Primitive::ui();

    float radius = _radius;
    if(ImGui::InputFloat("Radius", &radius))
        setRadius(radius);
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

void Plane::ui()
{
    Primitive::ui();

    float scale = _scale;
    if(ImGui::InputFloat("Scale", &scale))
        setScale(scale);
}

}
