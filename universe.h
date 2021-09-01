#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <vector>
#include <chrono>

// To prevent glfw3.h from including gl.h
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "body.h"
#include "camera.h"
#include "gravity.h"
#include "radiation.h"


namespace unisim
{

class Universe
{
private:
    Universe();

public:
    static Universe& getInstance();

    int launch(int argc, char ** argv);

    CameraMan& cameraMan();

    double timeFactor() const;
    void setTimeFactor(double tf);

    void setup();
    void update();
    void draw();

public:
    int _argc;
    char** _argv;
    GLFWwindow* window;

    // Systems
    Gravity _gravity;
    Radiation _radiation;

    // Matter
    std::vector<Body*> _bodies;

    // Time
    double _dt;
    double _timeFactor;
    std::chrono::time_point<std::chrono::high_resolution_clock> _lastTime;

    // Camera
    Camera _camera;
    CameraMan _cameraMan;
};



// IMPLEMENTATION //
inline CameraMan& Universe::cameraMan()
{
    return _cameraMan;
}

inline double Universe::timeFactor() const
{
    return _timeFactor;
}

}

#endif // UNIVERSE_H
