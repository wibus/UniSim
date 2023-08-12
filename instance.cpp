#include "instance.h"

#include "GLM/gtc/constants.hpp"
#include "GLM/gtx/transform.hpp"

#include "units.h"

#include "body.h"


namespace unisim
{

Instance::Instance(
        const std::string& name,
        const std::shared_ptr<Body>& body,
        const std::vector<std::shared_ptr<Primitive>>& primitives,
        Instance* parent) :
    _parent(parent),
    _name(name),
    _body(body),
    _primitives(primitives)
{

}

Instance::~Instance()
{

}

void Instance::setBody(const std::shared_ptr<Body>& body)
{
    _body = body;
}

}
