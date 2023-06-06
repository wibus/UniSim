#include "camera.h"

#include "GLM/gtx/transform.hpp"


namespace unisim
{

Camera::Camera(Viewport viewport) :
    _viewport(viewport),
    _position(0, 0, 0),
    _lookAt(0, -1, 0),
    _up(0, 0, 1),
    _fov(45.0 / 180.0 * glm::pi<double>()),
    _exposure(1)
{
}

void Camera::setViewport(Viewport viewport)
{
    _viewport = viewport;
}

void Camera::setExposure(float exp)
{
    _exposure = exp;
}

void Camera::setFieldOfView(float fov)
{
    _fov = fov;
}

void Camera::setPosition(const glm::dvec3& pos)
{
    _position = pos;
}

void Camera::setLookAt(const glm::dvec3& lookAt)
{
    _lookAt = lookAt;
}

void Camera::setUp(const glm::dvec3& up)
{
    _up = up;
}

glm::dmat4 Camera::view() const
{
   return glm::lookAt(
       glm::dvec3(_position.x, _position.y, _position.z),
       glm::dvec3(_lookAt.x,   _lookAt.y,   _lookAt.z),
       glm::dvec3(_up.x,       _up.y,       _up.z));
}

glm::mat4 Camera::proj() const
{
    return glm::perspective(
        _fov,
        _viewport.width / (float) _viewport.height,
        1.0f,
        2.0f);
}

glm::mat4 Camera::screen() const
{
    return
        glm::scale(glm::vec3(_viewport.width, _viewport.height, 1.0)) *
        glm::scale(glm::vec3(0.5, 0.5, 1.0)) *
        glm::translate(glm::vec3(1, 1, 0));
}

CameraMan::CameraMan(Viewport viewport) :
    _camera(viewport)
{
}

void CameraMan::setViewport(Viewport viewport)
{
    _camera.setViewport(viewport);
}

void CameraMan::handleKeyboard(const Inputs& inputs, const KeyboardEvent& event)
{

}

void CameraMan::handleMouseMove(const Inputs& inputs, const MouseMoveEvent& event)
{

}

void CameraMan::handleMouseButton(const Inputs& inputs, const MouseButtonEvent& event)
{

}

void CameraMan::handleMouseScroll(const Inputs& inputs, const MouseScrollEvent& event)
{

}

}
