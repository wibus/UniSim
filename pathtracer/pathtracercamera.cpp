#include "pathtracercamera.h"

#include <iostream>

#include <GLFW/glfw3.h>

#include "GLM/gtx/transform.hpp"

#include "../units.h"
#include "../object.h"
#include "../body.h"
#include "../material.h"
#include "../scene.h"


namespace unisim
{

const float PathTracerCameraMan::ZOOM_INC = 0.1;
const float PathTracerCameraMan::EXPOSURE_INC = glm::sqrt(2.0);
const float PathTracerCameraMan::ROTATE_INC = glm::pi<float>() * 0.01;
const float PathTracerCameraMan::APPROACH_INC = 1.01;

PathTracerCameraMan::PathTracerCameraMan(Scene& scene, Viewport viewport) :
    CameraMan(viewport),
    _scene(scene),
    _position(0, 0, 0),
    _direction(0, 1, 0),
    _up(0, 0, 1),
    _speed(1),
    _pan(0, 0, 0, 1),
    _tilt(0, 0, 0, 1),
    _autoExpose(true)
{
    _camera.setViewport(viewport);
    _camera.setExposure(1);

    _position = glm::dvec3(0, -5, 5);
}

void PathTracerCameraMan::update(const Inputs& inputs, float dt)
{
    if(inputs.keyboard().keyPressed[GLFW_KEY_W])
        moveForward();
    if(inputs.keyboard().keyPressed[GLFW_KEY_S])
        moveBackward();
    if(inputs.keyboard().keyPressed[GLFW_KEY_A])
        strafeLeft();
    if(inputs.keyboard().keyPressed[GLFW_KEY_D])
        strafeRight();
    if(inputs.keyboard().keyPressed[GLFW_KEY_E])
        strafeUp();
    if(inputs.keyboard().keyPressed[GLFW_KEY_Q])
        strafeDown();

    _direction = rotatePoint(quatMul(_pan, _tilt), glm::vec3(0, 1, 0));

    _camera.setUp(_up);
    _camera.setPosition(_position);
    _camera.setLookAt(_position + _direction);
}

void PathTracerCameraMan::handleKeyboard(const Inputs& inputs, const KeyboardEvent& event)
{
    if(event.action == GLFW_PRESS)
    {
        if(event.key == GLFW_KEY_KP_ADD)
        {
            exposeUp();
        }
        else if(event.key == GLFW_KEY_KP_SUBTRACT)
        {
            exposeDown();
        }
    }
}

void PathTracerCameraMan::handleMouseMove(const Inputs& inputs, const MouseMoveEvent& event)
{
    int dx = event.xpos - _lastMouseMove.xpos;
    int dy = event.ypos - _lastMouseMove.ypos;
    _lastMouseMove = event;

    if(inputs.mouse().buttonPressed[GLFW_MOUSE_BUTTON_LEFT])
    {
        rotatePrimary(dx, dy);
    }
    else if(inputs.mouse().buttonPressed[GLFW_MOUSE_BUTTON_RIGHT])
    {
        rotateSecondary(dx, dy);
    }
}

void PathTracerCameraMan::handleMouseButton(const Inputs& inputs, const MouseButtonEvent& event)
{

}

void PathTracerCameraMan::handleMouseScroll(const Inputs& inputs, const MouseScrollEvent& event)
{
    if(event.yoffset != 0)
    {
        if(event.yoffset > 0)
        {
            zoomIn();
        }
        else if(event.yoffset < 0)
        {
            zoomOut();
        }
    }
}

void PathTracerCameraMan::orbit(int newBodyId, int oldBodyId)
{

}

void PathTracerCameraMan::zoomIn()
{
    float factor = 4.0 * (_camera.fieldOfView() * (glm::pi<float>() - _camera.fieldOfView())) / (glm::pi<float>() * glm::pi<float>());
    float zoom = 1 + (ZOOM_INC * glm::sqrt(factor));
    _camera.setFieldOfView(glm::mix(0.0f, _camera.fieldOfView(), 1 / zoom));
}

void PathTracerCameraMan::zoomOut()
{
    float factor = 4.0 * (_camera.fieldOfView() * (glm::pi<float>() - _camera.fieldOfView())) / (glm::pi<float>() * glm::pi<float>());
    float zoom = 1 + (ZOOM_INC * glm::sqrt(factor));
    _camera.setFieldOfView(glm::mix(0.0f, _camera.fieldOfView(), zoom));
}


void PathTracerCameraMan::exposeUp()
{
    _camera.setExposure(_camera.exposure() * EXPOSURE_INC);
}

void PathTracerCameraMan::exposeDown()
{
    _camera.setExposure(_camera.exposure() / EXPOSURE_INC);
}

void PathTracerCameraMan::enableAutoExposure(bool enabled)
{
    _autoExpose = enabled;
}


void PathTracerCameraMan::rotatePrimary(int dx, int dy)
{
    float factor = _camera.fieldOfView() / glm::pi<float>();
    _pan   = quatMul(quat(glm::dvec3(0, 0, 1), -ROTATE_INC * dx * factor), _pan);
    _tilt  = quatMul(quat(glm::dvec3(1, 0, 0), -ROTATE_INC * dy * factor), _tilt);
}

void PathTracerCameraMan::rotateSecondary(int dx, int dy)
{
}


void PathTracerCameraMan::moveForward()
{
    _position += _direction * _speed;
}

void PathTracerCameraMan::moveBackward()
{
    _position -= _direction * _speed;
}

void PathTracerCameraMan::strafeLeft()
{
    glm::dvec3 right = glm::normalize(glm::cross(_direction, _up));
    _position -= right * _speed;
}

void PathTracerCameraMan::strafeRight()
{
    glm::dvec3 right = glm::normalize(glm::cross(_direction, _up));
    _position += right * _speed;
}

void PathTracerCameraMan::strafeUp()
{
    _position += _up * _speed;
}

void PathTracerCameraMan::strafeDown()
{
    _position -= _up * _speed;
}

}
