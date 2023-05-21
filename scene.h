#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>
#include <memory>


namespace unisim
{

class Object;


class Scene
{
public:
    Scene();
    virtual ~Scene();

    std::vector<std::shared_ptr<Object>>& objects();
    const std::vector<std::shared_ptr<Object>>& objects() const;

protected:

    // Matter
    std::vector<std::shared_ptr<Object>> _objects;
};


// IMPLEMENTATION //
inline std::vector<std::shared_ptr<Object>>& Scene::objects()
{
    return _objects;
}

inline const std::vector<std::shared_ptr<Object>>& Scene::objects() const
{
    return _objects;
}

}

#endif // SCENE_H
