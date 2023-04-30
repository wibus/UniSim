#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>


namespace unisim
{

class Material;
class Body;


class Scene
{
public:
    Scene();
    virtual ~Scene();

    std::vector<Material>& materials();
    const std::vector<Material>& materials() const;

    std::vector<Body*>& bodies();
    const std::vector<Body*>& bodies() const;

protected:

    // Materials
    std::vector<Material> _materials;

    // Matter
    std::vector<Body*> _bodies;
};


// IMPLEMENTATION //
inline std::vector<Material>& Scene::materials()
{
    return _materials;
}

inline const std::vector<Material>& Scene::materials() const
{
    return _materials;
}

inline std::vector<Body*>& Scene::bodies()
{
    return _bodies;
}

inline const std::vector<Body*>& Scene::bodies() const
{
    return _bodies;
}

}

#endif // SCENE_H
