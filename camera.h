#ifndef CAMERA_H
#define CAMERA_H

#include <vector>

#include "GLM/glm.hpp"


namespace unisim
{

class Body;

class Camera
{
public:
    Camera();

    void setResolution(int width, int height);

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
    int _width;
    int _height;

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

    CameraMan(Camera& camera, const std::vector<Body*>& bodies);

    void update(double dt);

    void setResolution(int width, int height);

    void setMode(Mode mode);
    void setBodyIndex(int bodyId);
    void setDistance(double dist);

    void zoomIn();
    void zoomOut();

    void exposeUp();
    void exposeDown();
    void enableAutoExposure(bool enabled);

    void rotatePrimary(int dx, int dy);
    void rotateSecondary(int dx, int dy);
    void moveForward();
    void moveBackward();
    void strafeLeft();
    void strafeRight();
    void strafeUp();
    void strafeDown();

private:
    void orbit(int newBodyId, int oldBodyId);

    Camera& _camera;
    const std::vector<Body*>& _bodies;

    Mode _mode;
    int _bodyId;

    glm::dvec3 _position;
    glm::dvec3 _direction;

    // Orbit/Ground
    double _distance;
    glm::dvec4 _longitude;
    glm::dvec4 _latitude;

    // Ground
    glm::dvec4 _pan;
    glm::dvec4 _tilt;
    glm::dvec4 _roam;

    bool _autoExpose;
};



// IMPLEMENTATION //
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

}

#endif // CAMERA_H
