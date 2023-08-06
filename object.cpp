#include "object.h"

#include "GLM/gtc/constants.hpp"
#include "GLM/gtx/transform.hpp"

#include "units.h"

#include "body.h"


namespace unisim
{

Object::Object(
        const std::string& name,
        const std::shared_ptr<Body>& body,
        const std::vector<std::shared_ptr<Primitive>>& primitives,
        Object* parent) :
    _parent(parent),
    _name(name),
    _body(body),
    _primitives(primitives)
{

}

Object::~Object()
{

}

void Object::setBody(const std::shared_ptr<Body>& body)
{
    _body = body;
}

}
