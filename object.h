#ifndef OBJECT_H
#define OBJECT_H

#include <GLM/glm.hpp>

#include <string>
#include <memory>

namespace unisim
{

class Body;
class Mesh;
class Material;

class Object
{
public:
    Object(const std::string& name,
           const std::shared_ptr<Body>& body,
           const std::shared_ptr<Mesh>& mesh,
           const std::shared_ptr<Material>& materal,
           Object* parent);

    virtual ~Object();

    std::string name() const { return _name; }

    void setBody(const std::shared_ptr<Body>& body);
    std::shared_ptr<Body> body() const { return _body; }

    void setMesh(const std::shared_ptr<Mesh>& mesh);
    std::shared_ptr<Mesh> mesh() const { return _mesh; }

    void setMaterial(const std::shared_ptr<Material>& material);
    std::shared_ptr<Material> material() const { return _material; }

private:
    Object* _parent;

    std::string _name;

    std::shared_ptr<Body> _body;

    std::shared_ptr<Mesh> _mesh;

    std::shared_ptr<Material> _material;
};

}

#endif // OBJECT_H
