#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <string>
#include <vector>
#include <memory>

#include <GLM/glm.hpp>

#include "units.h"


namespace unisim
{

class Material;

using Index = uint16_t;

struct Vertex
{
    glm::vec3 position;
    glm::vec3 nornal;
    glm::vec2 uv;
};

struct Triangle
{
    Index v[3];
};


class Primitive
{
public:
    enum Type
    {
        Mesh,
        Sphere,
        Plane,
        Type_Count
    };

    static const char* Type_Names[Type_Count];

protected:
    Primitive(Type type);

public:
    virtual ~Primitive();

    Type type() const { return _type; }

    std::shared_ptr<Material> material() const { return _material; }
    void setMaterial(const std::shared_ptr<Material>& material) { _material = material; }

    virtual void ui();

private:
    Type _type;
    std::shared_ptr<Material> _material;
};


class Mesh : public Primitive
{
public:
    Mesh();
    Mesh(const std::string& fileName);

    bool load(const std::string& fileName);

    const std::vector<Vertex>& vertices() const { return _vertices; }
    const std::vector<Triangle>& triangles() const { return _triangles; }

    static Mesh cube(float length, float uvScale);

    void ui() override;

private:
    std::vector<Vertex> _vertices;
    std::vector<Triangle> _triangles;
};


class Sphere : public Primitive
{
public:
    Sphere();
    Sphere(float radius);

    float radius() const { return _radius; }
    void setRadius(float radius) { _radius = radius; }

    void ui() override;

private:
    float _radius;
};


class Plane : public Primitive
{
public:
    Plane();
    Plane(float scale);

    float scale() const { return _scale; }
    void setScale(float scale) { _scale = scale; }

    void ui() override;

private:
    float _scale;
};

}

#endif // PRIMITIVE_H
