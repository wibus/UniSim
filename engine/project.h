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

    CameraMan& cameraMan(int index = 0);
    const CameraMan& cameraMan(int index = 0) const;

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

inline CameraMan& Project::cameraMan(int index)
{
    return *_cameraMen[index];
}

inline const CameraMan& Project::cameraMan(int index) const
{
    return *_cameraMen[index];
}

}

#endif // PROJECT_H
