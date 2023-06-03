#ifndef PROJECT_H
#define PROJECT_H

#include <memory>
#include <vector>

#include "camera.h"


namespace unisim
{

class Scene;
class CameraMan;
class Ui;


class Project
{
public:
    Project();
    ~Project();

    Scene& scene();
    const Scene& scene() const;

    virtual int addView(Viewport viewport) = 0;

    CameraMan& cameraMan(int index = 0);
    const CameraMan& cameraMan(int index = 0) const;

    Ui& ui();
    const Ui& ui() const;

protected:
    void reset(Scene* scene, Ui* ui);
    int addCameraMan(CameraMan* cameraMan);

private:
    std::unique_ptr<Scene> _scene;

    std::vector<std::shared_ptr<CameraMan>> _cameraMen;

    std::unique_ptr<Ui> _ui;
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

inline Ui& Project::ui()
{
    return *_ui;
}

inline const Ui& Project::ui() const
{
    return *_ui;
}

}

#endif // PROJECT_H
