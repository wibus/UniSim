#ifndef PROJECT_H
#define PROJECT_H

#include <memory>
#include <vector>

#include "camera.h"


namespace unisim
{

class Scene;


class Project
{
public:
    Project();
    virtual ~Project();

    Scene& scene();
    const Scene& scene() const;

    virtual int addView(const std::shared_ptr<View>& view) = 0;

    unsigned int cameraManCount() const;
    CameraMan& cameraMan(unsigned int index = 0);
    const CameraMan& cameraMan(unsigned int index = 0) const;

protected:
    void setScene(Scene* scene);
    int addCameraMan(CameraMan* cameraMan);

private:
    std::unique_ptr<Scene> _scene;

    std::vector<std::shared_ptr<CameraMan>> _cameraMen;
};


// IMPLEMENTATION //

inline Scene& Project::scene()
{
    return *_scene;
}

inline const Scene& Project::scene() const
{
    return *_scene;
}

inline unsigned int Project::cameraManCount() const
{
    return _cameraMen.size();
}

inline CameraMan& Project::cameraMan(unsigned int index)
{
    return *_cameraMen[index];
}

inline const CameraMan& Project::cameraMan(unsigned int index) const
{
    return *_cameraMen[index];
}

}

#endif // PROJECT_H
