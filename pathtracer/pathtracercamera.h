#ifndef PATH_TRACER_CAMERA_H
#define PATH_TRACER_CAMERA_H

#include "../camera.h"


namespace unisim
{

class PathTracerCameraMan : public CameraMan
{
public:
    PathTracerCameraMan(Scene &scene, Viewport viewport);

    void update(const Inputs& inputs, float dt) override;

    void handleKeyboard(const Inputs& inputs, const KeyboardEvent& event) override;
    void handleMouseMove(const Inputs& inputs, const MouseMoveEvent& event) override;
    void handleMouseButton(const Inputs& inputs, const MouseButtonEvent& event) override;
    void handleMouseScroll(const Inputs& inputs, const MouseScrollEvent& event) override;

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

    static const float ZOOM_INC;
    static const float EXPOSURE_INC;
    static const float ROTATE_INC;
    static const float APPROACH_INC;

protected:
    void orbit(int newBodyId, int oldBodyId);

    const Scene& _scene;

    glm::dvec3 _position;
    glm::dvec3 _direction;
    glm::dvec3 _up;
    double _speed;

    glm::dvec4 _pan;
    glm::dvec4 _tilt;

    bool _autoExpose;

    MouseMoveEvent _lastMouseMove;
};


// IMPLEMENTATION //

}

#endif // PATH_TRACER_CAMERA_H
