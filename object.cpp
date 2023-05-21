#include "object.h"

#include "GLM/gtc/constants.hpp"
#include "GLM/gtx/transform.hpp"

#include "units.h"

#include "body.h"
#include "material.h"


namespace unisim
{

double charTodouble(char c) { return glm::clamp((c - 65) / float(122 - 65), 0.0f, 1.0f); }

Object::Object(
        const std::string& name,
        const std::shared_ptr<Body>& body,
        const std::shared_ptr<Mesh>& mesh,
        const std::shared_ptr<Material>& materal,
        Object* parent) :
    _parent(parent),
    _name(name),
    _body(body),
    _mesh(mesh),
    _material(materal)
{

}

Object::~Object()
{

}

void Object::setBody(const std::shared_ptr<Body>& body)
{
    _body = body;
}

void Object::setMesh(const std::shared_ptr<Mesh> &mesh)
{
    _mesh = mesh;
}

void Object::setMaterial(const std::shared_ptr<Material>& material)
{
    _material = material;
}

}
