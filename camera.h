#ifndef CAMERA_H
#define CAMERA_H

#include <vector>

#include "GLM/glm.hpp"

#include "input.h"


namespace unisim
{

class Body;
class Scene;


struct Viewport
{
    int width;
    int height;
};


class Camera
{
public:
    Camera(Viewport viewport);

    const Viewport& viewport() const;
    void setViewport(Viewport viewport);

    double exposure() const;
    void setExposure(double exp);

    double fieldOfView() const;
    void setFieldOfView(double fov);

    glm::dvec3 position() const;
    void setPosition(const glm::dvec3& pos);

    glm::dvec3 lookAt() const;
    void setLookAt(const glm::dvec3& lookAt);

    glm::dvec3 up() const;
    void setUp(const glm::dvec3& up);

    glm::dmat4 view() const;
    glm::mat4 proj() const;
    glm::mat4 screen() const;

private:
    Viewport _viewport;

    glm::dvec3 _position;
    glm::dvec3 _lookAt;
    glm::dvec3 _up;
    double _fov;

    double _exposure;
    bool _autoExposure;
};

class CameraMan
{
public:
    enum class Mode {Static, Orbit, Ground};

    CameraMan(Viewport viewport);

    const Camera& camera() const;

    void setViewport(Viewport viewport);

    virtual void update(const Inputs& inputs, double dt) = 0;

    virtual void handleKeyboard(const Inputs& inputs, const KeyboardEvent& event);
    virtual void handleMouseMove(const Inputs& inputs, const MouseMoveEvent& event);
    virtual void handleMouseButton(const Inputs& inputs, const MouseButtonEvent& event);
    virtual void handleMouseScroll(const Inputs& inputs, const MouseScrollEvent& event);

protected:
    Camera _camera;
};



// IMPLEMENTATION //
inline const Viewport& Camera::viewport() const
{
    return _viewport;
}

inline double Camera::exposure() const
{
    return _exposure;
}

inline double Camera::fieldOfView() const
{
    return _fov;
}

inline glm::dvec3 Camera::position() const
{
    return _position;
}

inline glm::dvec3 Camera::lookAt() const
{
    return _lookAt;
}

inline glm::dvec3 Camera::up() const
{
    return _up;
}

inline const Camera& CameraMan::camera() const
{
    return _camera;
}

}

#endif // CAMERA_H
