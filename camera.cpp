#include "camera.h"

#include "GL/gl.h"

#include "GLM/gtx/transform.hpp"

#include "body.h"
#include "units.h"


namespace unisim
{

Camera::Camera() :
    _width(500),
    _height(500),
    _position(0, 0, 0),
    _lookAt(0, -1, 0),
    _up(0, 0, 1),
    _fov(45.0 / 180.0 * glm::pi<double>()),
    _exposure(1)
{
}

void Camera::setResolution(int width, int height)
{
    _width = width;
    _height = height;
}

void Camera::setExposure(double exp)
{
    _exposure = exp;
}

void Camera::setFieldOfView(double fov)
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
        _width / (double) _height,
        1.0,
        2.0);
}

glm::mat4 Camera::screen() const
{
    return
        glm::scale(glm::vec3(_width, _height, 1.0)) *
        glm::scale(glm::vec3(0.5, 0.5, 1.0)) *
        glm::translate(glm::vec3(1, 1, 0));
}

CameraMan::CameraMan(Camera& camera, const std::vector<Body*>& bodies) :
    _camera(camera),
    _bodies(bodies),
    _mode(Mode::Static),
    _position(0, 0, 500e9),
    _direction(0, 0, -1e6),
    _distance(1e6),
    _longitude(0, 0, 0, 1),
    _latitude(0, 0, 0, 1),
    _pan(0, 0, 0, 1),
    _tilt(0, 0, 0, 1),
    _roam(0, 0, 0, 1),
    _autoExpose(true)
{
}

void CameraMan::update(double dt)
{
    switch(_mode)
    {
    case Mode::Static:
        _camera.setUp(glm::dvec3(0, 1, 0));
        _camera.setPosition(_position);
        _camera.setLookAt(_position + _direction);
        break;
    case Mode::Orbit:
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            glm::dvec3 bodyPos = _bodies[_bodyId]->position();
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
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            glm::dvec3 front(0, 0, _distance);
            glm::dvec3 up   (0, _distance, 0);

            glm::dvec4 bodyQuat = _bodies[_bodyId]->quaternion();
            glm::dvec3 bodyPos = _bodies[_bodyId]->position();

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

void CameraMan::setResolution(int width, int height)
{
    _camera.setResolution(width, height);
}

void CameraMan::setMode(Mode mode)
{
    _mode = mode;

    if(_mode == Mode::Orbit || _mode == Mode::Ground)
        orbit(_bodyId, _bodyId);
}

void CameraMan::setBodyIndex(int bodyId)
{
    int oldBodyId = _bodyId;
    _bodyId = bodyId;

    if(_mode == Mode::Orbit || _mode == Mode::Ground)
        orbit(bodyId, oldBodyId);
}

void CameraMan::setDistance(double distance)
{
    _distance = distance;
}

void CameraMan::orbit(int newBodyId, int oldBodyId)
{
    if(_bodyId != -1 && _bodyId < _bodies.size())
    {
        Body* body = _bodies[newBodyId];
        _latitude = glm::dvec4(0, 0, 0, 1);
        _longitude = glm::dvec4(0, 0, 0, 1);
        _pan = glm::dvec4(0, 0, 0, 1);
        _tilt = glm::dvec4(0, 0, 0, 1);
        _roam = glm::dvec4(0, 0, 0, 1);

        if(_mode == Mode::Ground)
            _distance = 10.0 * body->radius();
        else
        {
            if(_distance <= body->radius() * 1.1)
                _distance = body->radius() * 1.1;
        }

        if(_autoExpose && oldBodyId != newBodyId)
        {
            if(oldBodyId != -1 && oldBodyId < _bodies.size())
            {
                if(!(glm::any(glm::bvec3(_bodies[oldBodyId]->emission())) ||
                     glm::any(glm::bvec3(_bodies[newBodyId]->emission()))))
                {
                glm::dvec3 oldPos = _bodies[oldBodyId]->position();
                double oldRelIrradiance = 1.0 / glm::dot(oldPos, oldPos);

                glm::dvec3 newPos = _bodies[newBodyId]->position();
                double newRelIrradiance = 1.0 / glm::dot(newPos, newPos);

                _camera.setExposure(_camera.exposure() * oldRelIrradiance / newRelIrradiance);
                }
            }
        }
    }
}

const double ZOON_INC = 0.1;

void CameraMan::zoomIn()
{
    switch(_mode)
    {
    case Mode::Static:
        _position += _direction * 1000.0;
        break;
    case Mode::Orbit:
    case Mode::Ground:
    {
        double factor = 4.0 * (_camera.fieldOfView() * (glm::pi<double>() - _camera.fieldOfView())) / (glm::pi<double>() * glm::pi<double>());
        double zoom = 1 + (ZOON_INC * glm::sqrt(factor));
        _camera.setFieldOfView(glm::mix(0.0, _camera.fieldOfView(), 1 / zoom));
    }
        break;
    }
}

void CameraMan::zoomOut()
{
    switch(_mode)
    {
    case Mode::Static:
        _position -= _direction * 1000.0;
        break;
    case Mode::Orbit:
    case Mode::Ground:
    {
        double factor = 4.0 * (_camera.fieldOfView() * (glm::pi<double>() - _camera.fieldOfView())) / (glm::pi<double>() * glm::pi<double>());
        double zoom = 1 + (ZOON_INC * glm::sqrt(factor));
        _camera.setFieldOfView(glm::mix(0.0, _camera.fieldOfView(), zoom));
    }
        break;
    }
}

const double EXPOSURE_INC = glm::sqrt(2.0);

void CameraMan::exposeUp()
{
    _camera.setExposure(_camera.exposure() * EXPOSURE_INC);
}

void CameraMan::exposeDown()
{
    _camera.setExposure(_camera.exposure() / EXPOSURE_INC);
}

void CameraMan::enableAutoExposure(bool enabled)
{
    _autoExpose = enabled;
}

const double ROTATE_INC = glm::pi<double>() * 0.001;

void CameraMan::rotatePrimary(int dx, int dy)
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

void CameraMan::rotateSecondary(int dx, int dy)
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

double APPROACH_INC = 1.01;

void CameraMan::moveForward()
{
    switch(_mode)
    {
    case Mode::Static:
        _position += _direction * 1000.0;
        break;
    case Mode::Orbit:        
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            Body* body = _bodies[_bodyId];
            _distance = glm::mix(body->radius(),  _distance, 1 / APPROACH_INC);
        }
        break;
    case Mode::Ground:
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            Body* body = _bodies[_bodyId];
            double moveScale = (_distance - body->radius()) / _distance;

            glm::dvec4 camQuat = quatMul(_roam, _pan);
            glm::dvec3 rotAxis = rotatePoint(camQuat, glm::dvec3(1, 0, 0));
            double rotAngle = glm::pi<double>() * 0.01 * moveScale;
            glm::dvec4 rotQuat = quat(rotAxis, rotAngle);

            _roam = quatMul(rotQuat, _roam);

            break;
        }
    }
}

void CameraMan::moveBackward()
{
    switch(_mode)
    {
    case Mode::Static:
        _position -= _direction * 1000.0;
        break;
    case Mode::Orbit:        
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            Body* body = _bodies[_bodyId];
            _distance = glm::mix(body->radius(),  _distance, APPROACH_INC);
        }
        break;
    case Mode::Ground:
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            Body* body = _bodies[_bodyId];
            double moveScale = (_distance - body->radius()) / _distance;

            glm::dvec4 camQuat = quatMul(_roam, _pan);
            glm::dvec3 rotAxis = rotatePoint(camQuat, glm::dvec3(1, 0, 0));
            double rotAngle = -glm::pi<double>() * 0.01 * moveScale;
            glm::dvec4 rotQuat = quat(rotAxis, rotAngle);

            _roam = quatMul(rotQuat, _roam);

            break;
        }
    }
}

void CameraMan::strafeLeft()
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        break;
    case Mode::Ground:
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            Body* body = _bodies[_bodyId];
            double moveScale = (_distance - body->radius()) / _distance;

            glm::dvec4 camQuat = quatMul(_roam, _pan);
            glm::dvec3 rotAxis = rotatePoint(camQuat, glm::dvec3(0, 0, 1));
            double rotAngle = -glm::pi<double>() * 0.01 * moveScale;
            glm::dvec4 rotQuat = quat(rotAxis, rotAngle);

            _roam = quatMul(rotQuat, _roam);

            break;
        }
    }
}

void CameraMan::strafeRight()
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        break;
    case Mode::Ground:
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            Body* body = _bodies[_bodyId];
            double moveScale = (_distance - body->radius()) / _distance;

            glm::dvec4 camQuat = quatMul(_roam, _pan);
            glm::dvec3 rotAxis = rotatePoint(camQuat, glm::dvec3(0, 0, 1));
            double rotAngle = glm::pi<double>() * 0.01 * moveScale;
            glm::dvec4 rotQuat = quat(rotAxis, rotAngle);

            _roam = quatMul(rotQuat, _roam);

            break;
        }
    }
}

void CameraMan::strafeUp()
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        break;
    case Mode::Ground:
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            Body* body = _bodies[_bodyId];
            _distance = glm::mix(body->radius(),  _distance, APPROACH_INC);
        }
        break;
    }
}

void CameraMan::strafeDown()
{
    switch(_mode)
    {
    case Mode::Static:
        break;
    case Mode::Orbit:
        break;
    case Mode::Ground:
        if(_bodyId != -1 && _bodyId < _bodies.size())
        {
            Body* body = _bodies[_bodyId];
            _distance = glm::mix(body->radius(),  _distance, 1 / APPROACH_INC);
        }
        break;
    }
}


}
