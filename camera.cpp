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
    _ev(1),
    _filmHeight(0.024),
    _focalLength(0.035),
    _aperture(16),
    _shutterSpeed(1 / 100.0f),
    _iso(100)
{
    updateEV();
    // Update field of view
    setFocalLength(_focalLength);
}

void Camera::setViewport(Viewport viewport)
{
    _viewport = viewport;
}

void Camera::setFilmHeight(float height)
{
    _filmHeight = height;

    // Update focal length
    setFieldOfView(_fov);
}

void Camera::setFocalLength(float length)
{
    _focalLength = length;

    _fov = 2 * glm::atan(_filmHeight / (2 * _focalLength));
}

void Camera::setFieldOfView(float fov)
{
    _fov = fov;

    _focalLength = _filmHeight / glm::tan(_fov * 0.5f) * 0.5f;
}

void Camera::setAperture(float aperture)
{
    _aperture = aperture;
    updateEV();
}

void Camera::setShutterSpeed(float speed)
{
    _shutterSpeed = speed;
    updateEV();
}

void Camera::setIso(float iso)
{
    _iso = iso;
    updateEV();
}

void Camera::setEV(float ev)
{
    _ev = ev;

    _iso = glm::exp2(_ev) * 100.0f * _shutterSpeed / (_aperture * _aperture);
}

float Camera::exposure() const
{
    float sunIlluminanceNoon = 30000;
    float sunny16Light = (16 * 16 * 100);
    return (sunny16Light / glm::exp2(_ev)) / sunIlluminanceNoon;
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

void Camera::updateEV()
{
    _ev = glm::log2(_aperture * _aperture * _iso / (100 * _shutterSpeed));
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
