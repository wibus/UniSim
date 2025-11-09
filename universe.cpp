#include "universe.h"

#include <iostream>

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>

#include <PilsCore/Utils/Logger.h>

#include "system/profiler.h"

#include "graphic/window.h"

#include "engine/scene.h"
#include "engine/project.h"
#include "engine/terrain.h"
#include "engine/sky.h"

#include "projects/solar/solarsystemproject.h"
#include "projects/pathtracer/pathtracerproject.h"


namespace unisim
{

DefineProfilePoint(PollEvents);
DefineProfilePoint(Update);
DefineProfilePoint(Draw);
DefineProfilePoint(Ui);
DefineProfilePoint(ImGui_NewFrame);
DefineProfilePoint(SwapBuffers);

DefineProfilePointGpu(SwapBuffers);

DeclareProfilePoint(Frame);
DeclareProfilePointGpu(Frame);
DeclareProfilePointGpu(PathTracer);


Universe::Universe() :
    _timeFactor(0.0),
    _showUi(false)
{
}

int Universe::launch()
{
    _mainWindow.reset(new Window());

    if (!_mainWindow->isValid())
    {
        PILS_ERROR("Could not create window. Exiting.");
        return -1;
    }

    _mainWindow->registerEventListener(this);

    bool ok = setup();

    while (!_mainWindow->shouldClose() && ok)
    {
        Profiler::GetInstance().swapFrames();

        {
            Profile(PollEvents);
            _mainWindow->pollEvents();
        }

        // Start ImGui frame
        {
            Profile(ImGui_NewFrame);
            _mainWindow->ImGuiNewFrame();
        }

        update();
        ui();
        draw();

        {
            Profile(SwapBuffers);
            ProfileGpu(SwapBuffers);
            _mainWindow->present();
        }
    }

    _mainWindow->unregisterEventListener(this);
    _mainWindow->close();

    return 0;
}

void Universe::onWindowResize(const Window& window, int width, int height)
{

}

void Universe::onWindowKeyboard(const Window& window, const KeyboardEvent& event)
{
    // Open Close ImGui Window
    if(event.action == GLFW_PRESS)
    {
        if(event.key == GLFW_KEY_F10)
        {
            _showUi = !_showUi;
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureKeyboard)
        return;

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

    _inputs.handleKeyboard(event);

    _project->cameraMan().handleKeyboard(_inputs, event);
}

void Universe::onWindowMouseMove(const Window& window, const MouseMoveEvent& event)
{
    _inputs.handleMouseMove(event);

    _project->cameraMan().handleMouseMove(_inputs, event);
}

void Universe::onWindowMouseButton(const Window& window, const MouseButtonEvent& event)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
        return;

    _inputs.handleMouseButton(event);

    _project->cameraMan().handleMouseButton(_inputs, event);
}

void Universe::onWindowMouseScroll(const Window& window, const MouseScrollEvent& event)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
        return;

    _inputs.handleMouseScroll(event);

    _project->cameraMan().handleMouseScroll(_inputs, event);
}

bool Universe::setup()
{
    Profiler::GetInstance().initize();

    _mainView.reset(new View(*_mainWindow));

    //_project.reset(new SolarSystemProject());
    _project.reset(new PathTracerProject());

    _project->addView(_mainView);

    auto& instances = _project->scene().instances();
    auto addInstnaces = [&](const std::vector<std::shared_ptr<Instance>>& o)
        {instances.insert(instances.end(), o.begin(), o.end());};
    addInstnaces(_project->scene().terrain()->instances());

    bool ok = true;

    ok = ok && _graphic.initialize(
                _project->scene(),
                _project->cameraMan().camera());

    ok = ok && _engine.initialize(
                _project->scene(),
                _project->cameraMan().camera(),
                _graphic.resources());

    _dt = 0;
    _lastTime = std::chrono::high_resolution_clock::now();

    return ok;
}

void Universe::update()
{
    Profile(Update);

    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeDiff = currentTime - _lastTime;
    _lastTime = currentTime;
    _dt = timeDiff.count();

    _dt *= 60 * 60 * 24 * _timeFactor;

    _project->cameraMan().update(_inputs, _dt);

    _engine.execute(
        _dt,
        _project->scene(),
        _project->cameraMan().camera(),
        _graphic.resources());
}

void Universe::draw()
{
    Profile(Draw);

    _graphic.execute(
        _project->scene(),
        _project->cameraMan().camera());
}

void Universe::ui()
{
    Profile(Ui);

    if(!_showUi)
        return;

    if(ImGui::Begin(_project->scene().name().c_str(), &_showUi))
    {
        if(ImGui::BeginTabBar("#tabs"))
        {
            if(ImGui::BeginTabItem("General"))
            {
                Camera& camera = _project->cameraMan().camera();

                float ev = camera.ev();
                if(ImGui::SliderFloat("EV", &ev, -6.0f, 17.0f))
                    camera.setEV(ev);

                SkyLocalization& local = _project->scene().sky()->localization();
                float timeOfDay = local.timeOfDay();
                if(ImGui::SliderFloat("Time of Day", &timeOfDay, 0, SkyLocalization::MAX_TIME_OF_DAY))
                    local.setTimeOfDay(timeOfDay);

                ImGui::Separator();

                ImGui::Text("CPU Time %.3gms   (Sync %.3gms)",
                            Profiler::GetInstance().getCpuPointNs(PID_CPU(Frame)) * 1e-6,
                            Profiler::GetInstance().getCpuSyncNs() * 1e-6);
                ImGui::Text("GPU Time %.3gms   (Sync %.3gms)",
                            Profiler::GetInstance().getGpuPointNs(PID_GPU(Frame)) * 1e-6,
                            Profiler::GetInstance().getGpuSyncNs() * 1e-6);
                ImGui::Text("Path Tracer %.3gms",
                            Profiler::GetInstance().getGpuPointNs(PID_GPU(PathTracer)) * 1e-6);

                ImGui::Separator();

                if(ImGui::Button("Reload Shaders"))
                {
                    std::cout << "\n** Reloading shaders ** \n\n";

                    bool ok = _graphic.reloadShaders(
                        _project->scene(),
                        _project->cameraMan().camera());

                    if(!ok)
                    {
                        std::cerr << "Failed to reload some shaders\n\n";
                    }
                    else
                    {
                        std::cout << "\n** Shaders reloaded ** \n" << std::endl;
                    }
                }

                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Camera"))
            {
                _project->cameraMan().camera().ui();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Scene"))
            {
                _project->scene().ui();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Profiler"))
            {
                Profiler::GetInstance().ui();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();
}

}
