#ifndef INPUT_H
#define INPUT_H


namespace unisim
{

// MOUSE //
///////////

struct Mouse
{
    Mouse()
    {
        for(int i = 0; i < BUTTON_COUNT; ++i)
            buttonPressed[i] = false;
    }

    double xpos = 0;
    double ypos = 0;

    static const int BUTTON_COUNT = 3;
    bool buttonPressed[BUTTON_COUNT];

    double xangle = 0;
    double yangle = 0;
};

struct MouseMoveEvent
{
    double xpos;
    double ypos;
};

struct MouseButtonEvent
{
    int button;
    int action;
    int mods;
};

struct MouseScrollEvent
{
    double xoffset;
    double yoffset;
};


// KEYBOARD //
//////////////

struct Keyboard
{
    Keyboard()
    {
        for(int i = 0; i < KEY_COUNT; ++i)
            keyPressed[i] = false;
    }

    static const int KEY_COUNT = 256;
    bool keyPressed[KEY_COUNT];
};

struct KeyboardEvent
{
    int key;
    int scancode;
    int action;
    int mods;
};

// INPUTS //
////////////

class Inputs
{
public:
    Inputs();

    const Mouse& mouse() const;

    const Keyboard& keyboard() const;

    void handleKeyboard(const KeyboardEvent& event);
    void handleMouseMove(const MouseMoveEvent& event);
    void handleMouseButton(const MouseButtonEvent& event);
    void handleMouseScroll(const MouseScrollEvent& event);

private:
    Mouse _mouse;
    Keyboard _keyboard;
};



// IMPLEMENTATION //
inline const Mouse& Inputs::mouse() const
{
    return _mouse;
}

inline const Keyboard& Inputs::keyboard() const
{
    return _keyboard;
}

}

#endif // INPUT_H
