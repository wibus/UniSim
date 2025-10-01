#include "input.h"

#include <GLFW/glfw3.h>


namespace unisim
{

Inputs::Inputs()
{

}

void Inputs::handleKeyboard(const KeyboardEvent& event)
{
    if(event.key < Keyboard::KEY_COUNT)
    {
        if(event.action == GLFW_PRESS)
        {
            _keyboard.keyPressed[event.key] = true;
        }
        else if(event.action == GLFW_RELEASE)
        {
            _keyboard.keyPressed[event.key] = false;
        }
    }
}

void Inputs::handleMouseMove(const MouseMoveEvent& event)
{
    _mouse.xpos = event.xpos;
    _mouse.ypos = event.ypos;
}

void Inputs::handleMouseButton(const MouseButtonEvent& event)
{
    if(event.button == GLFW_MOUSE_BUTTON_LEFT)
        _mouse.buttonPressed[GLFW_MOUSE_BUTTON_LEFT] = event.action == GLFW_PRESS;
    else if(event.button == GLFW_MOUSE_BUTTON_RIGHT)
        _mouse.buttonPressed[GLFW_MOUSE_BUTTON_RIGHT] = event.action == GLFW_PRESS;
}

void Inputs::handleMouseScroll(const MouseScrollEvent& event)
{
    _mouse.xangle += event.xoffset;
    _mouse.yangle += event.yoffset;
}

}
