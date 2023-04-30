#ifndef SOLAR_SYSTEM_CAMERA_MAN_H
#define SOLAR_SYSTEM_CAMERA_MAN_H

#include "../camera.h"


namespace unisim
{

class SolarSystemCameraMan : public CameraMan
{
public:
    SolarSystemCameraMan(Scene &scene, Viewport viewport);

    void update(const Inputs& inputs, double dt) override;

    void handleKeyboard(const Inputs& inputs, const KeyboardEvent& event) override;
    void handleMouseMove(const Inputs& inputs, const MouseMoveEvent& event) override;
    void handleMouseButton(const Inputs& inputs, const MouseButtonEvent& event) override;
    void handleMouseScroll(const Inputs& inputs, const MouseScrollEvent& event) override;


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

protected:
    void orbit(int newBodyId, int oldBodyId);


    const Scene& _scene;

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

    MouseMoveEvent _lastMouseMove;
};


// IMPLEMENTATION //

}

#endif // SOLAR_SYSTEM_CAMERA_MAN_H
