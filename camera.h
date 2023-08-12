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

    bool operator==(const Viewport& viewport) const
    {
        return viewport.width == width && viewport.height == height;
    }

    bool operator!=(const Viewport& viewport) const
    {
        return !(viewport == *this);
    }
};


class Camera
{
public:
    Camera(Viewport viewport);

    const Viewport& viewport() const;
    void setViewport(Viewport viewport);

    float filmHeight() const { return _filmHeight; }
    void setFilmHeight(float height);

    float focalLength() const { return _focalLength; }
    void setFocalLength(float length);

    float focusDistance() const { return _focusDistance; }
    void setFocusDistance(float distance) { _focusDistance = distance; }

    bool dofEnabled() const { return _dofEnabled; }
    void setDofEnabled(bool enabled) { _dofEnabled = enabled; }

    float fieldOfView() const;
    void setFieldOfView(float fov);

    float fstop() const { return _fstop; }
    void setFstop(float fstop);

    float shutterSpeed() const { return _shutterSpeed; }
    void setShutterSpeed(float speed);

    float iso() const { return _iso; }
    void setIso(float iso);

    float ev() const;
    void setEV(float ev);

    float exposure() const;

    glm::dvec3 position() const;
    void setPosition(const glm::dvec3& pos);

    glm::dvec3 lookAt() const;
    void setLookAt(const glm::dvec3& lookAt);

    glm::dvec3 up() const;
    void setUp(const glm::dvec3& up);

    glm::dvec3 direction() const;

    glm::dmat4 view() const;
    glm::mat4 proj() const;
    glm::mat4 screen() const;

    void ui();

private:
    void updateEV();

    Viewport _viewport;

    glm::dvec3 _position;
    glm::dvec3 _lookAt;
    glm::dvec3 _up;
    float _fov;

    float _ev;

    float _filmHeight;
    float _focalLength;
    float _focusDistance;
    bool _dofEnabled;
    float _fstop;
    float _shutterSpeed;
    float _iso;
};

class CameraMan
{
public:
    enum class Mode {Static, Orbit, Ground};

    CameraMan(Viewport viewport);

    Camera& camera();
    const Camera& camera() const;

    void setViewport(Viewport viewport);

    virtual void update(const Inputs& inputs, float dt) = 0;

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

inline float Camera::ev() const
{
    return _ev;
}

inline float Camera::fieldOfView() const
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

inline glm::dvec3 Camera::direction() const
{
    return glm::normalize(_lookAt - _position);
}

inline Camera& CameraMan::camera()
{
    return _camera;
}

inline const Camera& CameraMan::camera() const
{
    return _camera;
}

}

#endif // CAMERA_H
