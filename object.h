#ifndef OBJECT_H
#define OBJECT_H

#include <GLM/glm.hpp>

#include <string>
#include <memory>
#include <vector>


namespace unisim
{

class Body;
class Primitive;


class Object
{
public:
    Object(const std::string& name,
           const std::shared_ptr<Body>& body,
           const std::vector<std::shared_ptr<Primitive>>& primitives,
           Object* parent);

    virtual ~Object();

    const std::string& name() const { return _name; }

    void setBody(const std::shared_ptr<Body>& body);
    std::shared_ptr<Body> body() const { return _body; }

    const std::vector<std::shared_ptr<Primitive>>& primitives() const { return _primitives; }
    void setPrimitives(const std::vector<std::shared_ptr<Primitive>>& primitives) { _primitives = primitives; }
    void addPrimitives(const std::shared_ptr<Primitive>& primitive) { _primitives.push_back(primitive); }

private:
    Object* _parent;

    std::string _name;

    std::shared_ptr<Body> _body;

    std::vector<std::shared_ptr<Primitive>> _primitives;
};

}

#endif // OBJECT_H
