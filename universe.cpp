#include "universe.h"

#include <iostream>

#include <GLM/gtc/constants.hpp>

#include <GLFW/glfw3.h>

#include "units.h"
#include "material.h"
#include "scene.h"
#include "project.h"

#include "solar/solarsystemproject.h"


namespace unisim
{

Universe& Universe::getInstance()
{
    static Universe g_Instance;

    return g_Instance;
}

void glfWHandleWindowResize(GLFWwindow* window, int width, int height)
{
    Universe::getInstance().handleWindowResize(window, width, height);
}

void glfWHandleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    KeyboardEvent event = {key, scancode, action, mods};

    Universe::getInstance().handleKeyboard(event);
}

void glfWHandleMouseMove(GLFWwindow*, double xpos, double ypos)
{
    MouseMoveEvent event = {xpos, ypos};

    Universe::getInstance().handleMouseMove(event);
}

void glfWHandleMouseButton(GLFWwindow*, int button, int action, int mods)
{
    MouseButtonEvent event = {button, action, mods};

    Universe::getInstance().handleMouseButton(event);
}

void glfWHandleScroll(GLFWwindow*, double xoffset, double yoffset)
{
    MouseScrollEvent event = {xoffset, yoffset};

    Universe::getInstance().handleMouseScroll(event);
}

Universe::Universe() :
    _timeFactor(0.0)
{
    _project.reset(new SolarSystemProject());
}

int Universe::launch(int argc, char** argv)
{
    // Copy because glut is taking a pointer
    _argc = argc;
    _argv = argv;

    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    _viewport.width = 1280;
    _viewport.height = 720;

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    window = glfwCreateWindow(_viewport.width, _viewport.height, "Universe", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    glfwSwapInterval(0);

    glViewport(0, 0, _viewport.width, _viewport.height);

    glfwSetWindowSizeCallback(window, glfWHandleWindowResize);
    glfwSetKeyCallback(window, glfWHandleKeyboard);
    glfwSetCursorPosCallback(window, glfWHandleMouseMove);
    glfwSetMouseButtonCallback(window, glfWHandleMouseButton);
    glfwSetScrollCallback(window, glfWHandleScroll);

    setup();

    _gravity.initialize(_project->scene().bodies());
    _radiation.initialize(_project->scene().bodies());

    _dt = 0;
    _lastTime = std::chrono::high_resolution_clock::now();

    _project->addView(_viewport);

    while (!glfwWindowShouldClose(window))
    {
        update();
        draw();

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void Universe::handleWindowResize(GLFWwindow* window, int width, int height)
{
    _viewport.width = width;
    _viewport.height = height;

    glViewport(0, 0, width, height);

    _viewport = {width, height};
    _project->cameraMan().setViewport(_viewport);
}

void Universe::handleKeyboard(const KeyboardEvent& event)
{
    _inputs.handleKeyboard(event);

    if(event.action == GLFW_PRESS)
    {
        if(event.key == GLFW_KEY_SPACE)
        {
            if(_timeFactor == 1.0)
                _timeFactor = 0.0;
            else
                _timeFactor = 1.0;
        }
    }

    _project->cameraMan().handleKeyboard(_inputs, event);
}

void Universe::handleMouseMove(const MouseMoveEvent& event)
{
    _inputs.handleMouseMove(event);

    _project->cameraMan().handleMouseMove(_inputs, event);
}

void Universe::handleMouseButton(const MouseButtonEvent& event)
{
    _inputs.handleMouseButton(event);

    _project->cameraMan().handleMouseButton(_inputs, event);
}

void Universe::handleMouseScroll(const MouseScrollEvent& event)
{
    _inputs.handleMouseScroll(event);

    _project->cameraMan().handleMouseScroll(_inputs, event);
}

void Universe::setup()
{
}

void Universe::update()
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeDiff = currentTime - _lastTime;
    _lastTime = currentTime;
    _dt = timeDiff.count();

    _dt *= 60 * 60 * 24 * _timeFactor;

    _gravity.update(_project->scene().bodies(), _dt);
    _project->cameraMan().update(_inputs, _dt);
}

void Universe::draw()
{
    _radiation.draw(_project->scene().bodies(), _dt, _project->cameraMan().camera());
}

}
