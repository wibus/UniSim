#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>
#include <memory>

#include <GLM/glm.hpp>


namespace unisim
{

class Sky;
class Terrain;
class Instance;
class Camera;
class Texture;
class DirectionalLight;
class MaterialDatabase;


class Scene
{
public:
    Scene(const std::string& name);
    virtual ~Scene();

    const std::string& name() const { return _name; }

    std::shared_ptr<Sky> sky() const { return _sky; }

    std::shared_ptr<Terrain> terrain() const { return _terrain; }
    
    std::shared_ptr<MaterialDatabase> materialDb() const { return _materialDb; }

    std::vector<std::shared_ptr<Instance>>& instances() { return _instances; }
    const std::vector<std::shared_ptr<Instance>>& instances() const { return _instances; }

    void ui();

protected:
    std::string _name;

    std::shared_ptr<Sky> _sky;

    std::shared_ptr<Terrain> _terrain;

    std::vector<std::shared_ptr<Instance>> _instances;
    
    std::shared_ptr<MaterialDatabase> _materialDb;
};

}

#endif // SCENE_H
