#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <chrono>
#include <memory>

#include "system/input.h"

#include "graphic/window.h"

#include "engine/taskgraph/enginetask.h"
#include "engine/taskgraph/graphictaskgraph.h"

#include "engine/project.h"


namespace unisim
{

class Window;
class Project;


class Universe : public WindowEventListener
{
public:
    Universe();
    int launch();

    void onWindowResize(const Window& window, int width, int height) override;
    void onWindowKeyboard(const Window& window, const KeyboardEvent& event) override;
    void onWindowMouseMove(const Window& window, const MouseMoveEvent& event) override;
    void onWindowMouseButton(const Window& window, const MouseButtonEvent& event) override;
    void onWindowMouseScroll(const Window& window, const MouseScrollEvent& event) override;

    bool setup();
    void update();
    void draw();
    void ui();

public:
    std::shared_ptr<Window> _mainWindow;
    std::shared_ptr<View> _mainView;

    // Inputs
    Inputs _inputs;

    // Systems
    GraphicTaskGraph _graphic;
    EngineTaskGraph  _engine;

    // Time
    double _dt;
    double _timeFactor;
    std::chrono::time_point<std::chrono::high_resolution_clock> _lastTime;

    // Projet
    std::unique_ptr<Project> _project;

    // UI
    bool _showUi;
};



// IMPLEMENTATION //

}

#endif // UNIVERSE_H
