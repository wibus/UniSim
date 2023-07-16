#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>
#include <memory>

#include <GLM/glm.hpp>


namespace unisim
{

class Sky;
class Object;
class Camera;
class Texture;
class DirectionalLight;


class Scene
{
public:
    Scene(const std::string& name);
    virtual ~Scene();

    const std::string& name() const { return _name; }

    std::shared_ptr<Sky> sky() const { return _sky; }

    std::vector<std::shared_ptr<Object>>& objects() { return _objects; }
    const std::vector<std::shared_ptr<Object>>& objects() const { return _objects; }

    virtual void initializeCamera(Camera& camera);

protected:
    std::string _name;

    std::shared_ptr<Sky> _sky;

    std::vector<std::shared_ptr<Object>> _objects;
};

}

#endif // SCENE_H
