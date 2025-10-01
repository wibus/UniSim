#ifndef INSTANCE_H
#define INSTANCE_H

#include <GLM/glm.hpp>

#include <string>
#include <memory>
#include <vector>


namespace unisim
{

class Body;
class Primitive;


class Instance
{
public:
    Instance(const std::string& name,
           const std::shared_ptr<Body>& body,
           const std::vector<std::shared_ptr<Primitive>>& primitives,
           Instance* parent);

    virtual ~Instance();

    const std::string& name() const { return _name; }

    void setBody(const std::shared_ptr<Body>& body);
    std::shared_ptr<Body> body() const { return _body; }

    const std::vector<std::shared_ptr<Primitive>>& primitives() const { return _primitives; }
    void setPrimitives(const std::vector<std::shared_ptr<Primitive>>& primitives) { _primitives = primitives; }
    void addPrimitives(const std::shared_ptr<Primitive>& primitive) { _primitives.push_back(primitive); }

    void ui();

private:
    Instance* _parent;

    std::string _name;

    std::shared_ptr<Body> _body;

    std::vector<std::shared_ptr<Primitive>> _primitives;
};

}

#endif // INSTANCE_H
