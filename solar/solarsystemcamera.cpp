#include "solarsystemcamera.h"

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLM/gtx/transform.hpp"

#include "../units.h"
#include "../instance.h"
#include "../body.h"
#include "../primitive.h"
#include "../material.h"
#include "../scene.h"


namespace unisim
{

const float SolarSystemCameraMan::ZOON_INC = 0.1;
const float SolarSystemCameraMan::EV_INC = 0.5;
const float SolarSystemCameraMan::ROTATE_INC = glm::pi<float>() * 0.001;
const float SolarSystemCameraMan::APPROACH_INC = 1.01;

SolarSystemCameraMan::SolarSystemCameraMan(Scene& scene, Viewport viewport) :
    CameraMan(viewport),
    _scene(scene),
    _mode(Mode::Static),
    _position(0, 0, 500e9),
    _direction(0, 0, -1e6),
    _distance(1e6),
    _longitude(0, 0, 0, 1),
    _latitude(0, 0, 0, 1),
    _pan(0, 0, 0, 1),
    _tilt(0, 0, 0, 1),
    _roam(0, 0, 0, 1),
    _autoExpose(false)
{
    _camera.setViewport(viewport);
    _camera.setEV(15);
    setBodyIndex(3);
    setMode(SolarSystemCameraMan::Mode::Orbit);
    setDistance(2.5e7);
}

void SolarSystemCameraMan::update(const Inputs& inputs, float dt)
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

    switch(_mode)
    {
    case Mode::Static:
        _camera.setUp(glm::dvec3(0, 1, 0));
        _camera.setPosition(_position);
        _camera.setLookAt(_position + _direction);
        break;
    case Mode::Orbit:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            glm::dvec3 bodyPos = _scene.instances()[_instanceId]->body()->position();
            glm::dvec3 distance(0, - _distance, 0);
            glm::dvec4 camQuat = quatMul(_longitude, _latitude);
            glm::dvec3 camPos = rotatePoint(camQuat, distance);
            glm::dvec4 viewQuat = quatMul(_pan, _tilt);
            glm::dvec3 viewPos = rotatePoint(viewQuat, -camPos);
            _camera.setPosition(bodyPos + camPos);
            _camera.setLookAt(bodyPos + camPos + viewPos);
            _camera.setUp(glm::dvec3(0, 0, 1));
        }
        break;
    case Mode::Ground:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            glm::dvec3 front(0, 0, _distance);
            glm::dvec3 up   (0, _distance, 0);

            glm::dvec4 bodyQuat = _scene.instances()[_instanceId]->body()->quaternion();
            glm::dvec3 bodyPos = _scene.instances()[_instanceId]->body()->position();

            glm::dvec4 camQuat = quatMul(bodyQuat, _roam);
            glm::dvec3 camPos = rotatePoint(camQuat, up);

            glm::dvec4 viewQuat = quatMul(camQuat, quatMul(_pan, _tilt));
            glm::dvec3 viewPos = rotatePoint(viewQuat, front);

            _camera.setPosition(bodyPos + camPos);
            _camera.setLookAt(bodyPos + camPos + viewPos);
            _camera.setUp(camPos);
        }
        break;
    }
}

void SolarSystemCameraMan::handleKeyboard(const Inputs& inputs, const KeyboardEvent& event)
{
    static int moonDegrees = 0;

    if(event.action == GLFW_PRESS)
    {
        if(GLFW_KEY_0 <= event.key && event.key <= GLFW_KEY_9)
        {
            setBodyIndex(event.key - GLFW_KEY_0);
        }
        else if(event.key == GLFW_KEY_I)
        {
            setMode(SolarSystemCameraMan::Mode::Static);
        }
        else if(event.key == GLFW_KEY_O)
        {
            setMode(SolarSystemCameraMan::Mode::Orbit);
        }
        else if(event.key == GLFW_KEY_P)
        {
            setMode(SolarSystemCameraMan::Mode::Ground);
        }
        else if(event.key == GLFW_KEY_KP_ADD)
        {
            exposeUp();
        }
        else if(event.key == GLFW_KEY_KP_SUBTRACT)
        {
            exposeDown();
        }
        else if(event.key == GLFW_KEY_N)
        {
            std::cout << --moonDegrees << std::endl;
            Body& moon = *_scene.instances()[9]->body();
            moon.setQuaternion(quatMul(moon.quaternion(), quat(glm::dvec3(0, 0, -1), glm::pi<double>() / 180)));
        }
        else if(event.key == GLFW_KEY_M)
        {
            std::cout << ++moonDegrees << std::endl;
            Body& moon = *_scene.instances()[9]->body();
            moon.setQuaternion(quatMul(moon.quaternion(), quat(glm::dvec3(0, 0, 1), glm::pi<double>() / 180)));
        }
    }
}

void SolarSystemCameraMan::handleMouseMove(const Inputs& inputs, const MouseMoveEvent& event)
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

void SolarSystemCameraMan::handleMouseButton(const Inputs& inputs, const MouseButtonEvent& event)
{

}

void SolarSystemCameraMan::handleMouseScroll(const Inputs& inputs, const MouseScrollEvent& event)
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

void SolarSystemCameraMan::setMode(Mode mode)
{
    _mode = mode;

    if(_mode == Mode::Orbit || _mode == Mode::Ground)
        orbit(_instanceId, _instanceId);
}

void SolarSystemCameraMan::setBodyIndex(int bodyId)
{
    int oldBodyId = _instanceId;
    _instanceId = bodyId;

    if(_mode == Mode::Orbit || _mode == Mode::Ground)
        orbit(bodyId, oldBodyId);
}

void SolarSystemCameraMan::setDistance(float distance)
{
    _distance = distance;
}

void SolarSystemCameraMan::orbit(int newBodyId, int oldBodyId)
{
    if(_instanceId != -1 && _instanceId < _scene.instances().size())
    {
        const Sphere& sphere = static_cast<Sphere&>(*_scene.instances()[newBodyId]->primitives()[0]);
        _latitude = glm::dvec4(0, 0, 0, 1);
        _longitude = glm::dvec4(0, 0, 0, 1);
        _pan = glm::dvec4(0, 0, 0, 1);
        _tilt = glm::dvec4(0, 0, 0, 1);
        _roam = glm::dvec4(0, 0, 0, 1);

        if(_mode == Mode::Ground)
            _distance = 10.0 * sphere.radius();
        else
        {
            if(_distance <= sphere.radius() * 1.1)
                _distance = sphere.radius() * 1.1;
        }

        if(_autoExpose && oldBodyId != newBodyId)
        {
            if(oldBodyId != -1 && oldBodyId < _scene.instances().size())
            {
                const Sphere& oldSphere = static_cast<Sphere&>(*_scene.instances()[oldBodyId]->primitives()[0]);
                const Sphere& newSphere = static_cast<Sphere&>(*_scene.instances()[newBodyId]->primitives()[0]);

                if(!(glm::any(glm::bvec3(oldSphere.material()->defaultEmissionColor())) ||
                     glm::any(glm::bvec3(newSphere.material()->defaultEmissionColor()))))
                {
                    glm::dvec3 oldPos = _scene.instances()[oldBodyId]->body()->position();
                    float oldRelIrradiance = 1.0 / glm::dot(oldPos, oldPos);

                    glm::dvec3 newPos = _scene.instances()[newBodyId]->body()->position();
                    float newRelIrradiance = 1.0 / glm::dot(newPos, newPos);

                    _camera.setIso(_camera.iso() * newRelIrradiance / oldRelIrradiance);
                }
            }
        }
    }
}

void SolarSystemCameraMan::zoomIn()
{
    switch(_mode)
    {
    case Mode::Static:
        _position += _direction * 1000.0;
        break;
    case Mode::Orbit:
    case Mode::Ground:
    {
        float factor = 4.0 * (_camera.fieldOfView() * (glm::pi<float>() - _camera.fieldOfView())) / (glm::pi<float>() * glm::pi<float>());
        float zoom = 1 + (ZOON_INC * glm::sqrt(factor));
        _camera.setFieldOfView(glm::mix(0.0f, _camera.fieldOfView(), 1 / zoom));
    }
        break;
    }
}

void SolarSystemCameraMan::zoomOut()
{
    switch(_mode)
    {
    case Mode::Static:
        _position -= _direction * 1000.0;
        break;
    case Mode::Orbit:
    case Mode::Ground:
    {
        float factor = 4.0 * (_camera.fieldOfView() * (glm::pi<float>() - _camera.fieldOfView())) / (glm::pi<float>() * glm::pi<float>());
        float zoom = 1 + (ZOON_INC * glm::sqrt(factor));
        _camera.setFieldOfView(glm::mix(0.0f, _camera.fieldOfView(), zoom));
    }
        break;
    }
}


void SolarSystemCameraMan::exposeUp()
{
    _camera.setEV(_camera.ev() - EV_INC);
}

void SolarSystemCameraMan::exposeDown()
{
    _camera.setEV(_camera.ev() + EV_INC);
}

void SolarSystemCameraMan::enableAutoExposure(bool enabled)
{
    _autoExpose = enabled;
}


void SolarSystemCameraMan::rotatePrimary(int dx, int dy)
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        _longitude = quatMul(quat(glm::dvec3(0, 0, 1), -ROTATE_INC * dx), _longitude);
        _latitude  = quatMul(quat(glm::dvec3(1, 0, 0), -ROTATE_INC * dy), _latitude);
        break;
    case Mode::Ground:
    {
        double factor = _camera.fieldOfView() / glm::pi<double>();
        _pan   = quatMul(quat(glm::dvec3(0, 1, 0), -ROTATE_INC * dx * factor), _pan);
        _tilt  = quatMul(quat(glm::dvec3(1, 0, 0),  ROTATE_INC * dy * factor), _tilt);
    }
        break;
    }
}

void SolarSystemCameraMan::rotateSecondary(int dx, int dy)
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        _pan   = quatMul(quat(glm::dvec3(0, 0, 1), -ROTATE_INC * dx), _pan);
        _tilt  = quatMul(quat(glm::dvec3(1, 0, 0), -ROTATE_INC * dy), _tilt);
        break;
    case Mode::Ground:
        break;
    }
}


void SolarSystemCameraMan::moveForward()
{
    switch(_mode)
    {
    case Mode::Static:
        _position += _direction * 1000.0;
        break;
    case Mode::Orbit:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            const Sphere& sphere = static_cast<Sphere&>(*_scene.instances()[_instanceId]->primitives()[0]);
            _distance = glm::mix((double)sphere.radius(),  _distance, 1 / APPROACH_INC);
        }
        break;
    case Mode::Ground:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            const Sphere& sphere = static_cast<Sphere&>(*_scene.instances()[_instanceId]->primitives()[0]);
            double moveScale = (_distance - sphere.radius()) / _distance;

            glm::dvec4 camQuat = quatMul(_roam, _pan);
            glm::dvec3 rotAxis = rotatePoint(camQuat, glm::dvec3(1, 0, 0));
            double rotAngle = glm::pi<double>() * 0.01 * moveScale;
            glm::dvec4 rotQuat = quat(rotAxis, rotAngle);

            _roam = quatMul(rotQuat, _roam);

            break;
        }
    }
}

void SolarSystemCameraMan::moveBackward()
{
    switch(_mode)
    {
    case Mode::Static:
        _position -= _direction * 1000.0;
        break;
    case Mode::Orbit:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            const Sphere& sphere = static_cast<Sphere&>(*_scene.instances()[_instanceId]->primitives()[0]);
            _distance = glm::mix((double)sphere.radius(),  _distance, APPROACH_INC);
        }
        break;
    case Mode::Ground:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            const Sphere& sphere = static_cast<Sphere&>(*_scene.instances()[_instanceId]->primitives()[0]);
            double moveScale = (_distance - sphere.radius()) / _distance;

            glm::dvec4 camQuat = quatMul(_roam, _pan);
            glm::dvec3 rotAxis = rotatePoint(camQuat, glm::dvec3(1, 0, 0));
            double rotAngle = -glm::pi<double>() * 0.01 * moveScale;
            glm::dvec4 rotQuat = quat(rotAxis, rotAngle);

            _roam = quatMul(rotQuat, _roam);

            break;
        }
    }
}

void SolarSystemCameraMan::strafeLeft()
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        break;
    case Mode::Ground:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            const Sphere& sphere = static_cast<Sphere&>(*_scene.instances()[_instanceId]->primitives()[0]);
            double moveScale = (_distance - sphere.radius()) / _distance;

            glm::dvec4 camQuat = quatMul(_roam, _pan);
            glm::dvec3 rotAxis = rotatePoint(camQuat, glm::dvec3(0, 0, 1));
            double rotAngle = -glm::pi<double>() * 0.01 * moveScale;
            glm::dvec4 rotQuat = quat(rotAxis, rotAngle);

            _roam = quatMul(rotQuat, _roam);

            break;
        }
    }
}

void SolarSystemCameraMan::strafeRight()
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        break;
    case Mode::Ground:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            const Sphere& sphere = static_cast<Sphere&>(*_scene.instances()[_instanceId]->primitives()[0]);
            double moveScale = (_distance - sphere.radius()) / _distance;

            glm::dvec4 camQuat = quatMul(_roam, _pan);
            glm::dvec3 rotAxis = rotatePoint(camQuat, glm::dvec3(0, 0, 1));
            double rotAngle = glm::pi<double>() * 0.01 * moveScale;
            glm::dvec4 rotQuat = quat(rotAxis, rotAngle);

            _roam = quatMul(rotQuat, _roam);

            break;
        }
    }
}

void SolarSystemCameraMan::strafeUp()
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        break;
    case Mode::Ground:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            const Sphere& sphere = static_cast<Sphere&>(*_scene.instances()[_instanceId]->primitives()[0]);
            _distance = glm::mix((double)sphere.radius(),  _distance, APPROACH_INC);
        }
        break;
    }
}

void SolarSystemCameraMan::strafeDown()
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        break;
    case Mode::Ground:
        if(_instanceId != -1 && _instanceId < _scene.instances().size())
        {
            const Sphere& sphere = static_cast<Sphere&>(*_scene.instances()[_instanceId]->primitives()[0]);
            _distance = glm::mix((double)sphere.radius(),  _distance, 1 / APPROACH_INC);
        }
        break;
    }
}

}
