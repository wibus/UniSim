#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <vector>
#include <chrono>
#include <memory>

// To prevent glfw3.h from including gl.h
#include <GL/glew.h>

#include "camera.h"
#include "input.h"
#include "graphic.h"
#include "engine.h"


class GLFWwindow;

namespace unisim
{

class Project;


class Universe
{
private:
    Universe();

public:
    static Universe& getInstance();

    int launch(int argc, char ** argv);

    void handleWindowResize(GLFWwindow* window, int width, int height);
    void handleKeyboard(const KeyboardEvent& event);
    void handleMouseMove(const MouseMoveEvent& event);
    void handleMouseButton(const MouseButtonEvent& event);
    void handleMouseScroll(const MouseScrollEvent& event);

    bool setup();
    void update();
    void draw();
    void ui();

public:
    int _argc;
    char** _argv;
    GLFWwindow* window;

    // Inputs
    Inputs _inputs;

    // Systems
    GraphicTaskGraph _graphic;
    EngineTaskGraph  _engine;

    // Time
    double _dt;
    double _timeFactor;
    std::chrono::time_point<std::chrono::high_resolution_clock> _lastTime;

    // Viewport
    Viewport _viewport;

    // Projet
    std::unique_ptr<Project> _project;

    // UI
    bool _showUi;
};



// IMPLEMENTATION //

}

#endif // UNIVERSE_H
