#include "universe.h"

#include <iostream>

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "scene.h"
#include "project.h"
#include "profiler.h"
#include "terrain.h"
#include "sky.h"

#include "solar/solarsystemproject.h"
#include "pathtracer/pathtracerproject.h"


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

void glfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

Universe::Universe() :
    _timeFactor(0.0),
    _showUi(false)
{
}

int Universe::launch(int argc, char** argv)
{
    // Copy because glut is taking a pointer
    _argc = argc;
    _argv = argv;

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
        return -1;

    _viewport.width = 1280;
    _viewport.height = 720;

    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

    int monitorScore = -1;
    const int SMALL_MONITOR_SCORE = 1;
    const int LARGE_MONITOR_SCORE = 0;

    int viewportX = 0, viewportY = 0;
    GLFWmonitor* chosenMonitor = nullptr;

    for(int i = 0; i < monitorCount; ++i)
    {
        int xpos, ypos, width, height;
        glfwGetMonitorWorkarea(monitors[i], &xpos, &ypos, &width, &height);

        if(width <= _viewport.width || height <= 1024)
        {
            if(monitorScore < SMALL_MONITOR_SCORE)
            {
                chosenMonitor = monitors[i];
                monitorScore = SMALL_MONITOR_SCORE;

                _viewport.width = width;
                _viewport.height = height;
                viewportX = xpos;
                viewportY = ypos;
            }
        }
        else
        {
            if(monitorScore < LARGE_MONITOR_SCORE)
            {
                chosenMonitor = monitors[i];
                monitorScore = LARGE_MONITOR_SCORE;

                viewportX = xpos + (width - _viewport.width) / 2;
                viewportY = ypos + (height - _viewport.height) / 2;
            }
        }
    }

    if(!chosenMonitor)
    {
        return -2;
    }

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    GLFWwindow* window = glfwCreateWindow(_viewport.width, _viewport.height, "Universe", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwSetWindowPos(window, viewportX, viewportY);

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    glfwSwapInterval(1);

    glViewport(0, 0, _viewport.width, _viewport.height);

    glfwSetWindowSizeCallback(window, glfWHandleWindowResize);
    glfwSetKeyCallback(window, glfWHandleKeyboard);
    glfwSetCursorPosCallback(window, glfWHandleMouseMove);
    glfwSetMouseButtonCallback(window, glfWHandleMouseButton);
    glfwSetScrollCallback(window, glfWHandleScroll);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 440";
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool ok = setup();

    while (!glfwWindowShouldClose(window) && ok)
    {
        Profiler::GetInstance().swapFrames();

        {
            Profile(PollEvents);
            glfwPollEvents();
        }

        // Start ImGui frame
        {
            Profile(ImGui_NewFrame);
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        update();
        ui();
        draw();

        {
            Profile(SwapBuffers);
            ProfileGpu(SwapBuffers);
            glfwSwapBuffers(window);
        }
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

void Universe::handleMouseMove(const MouseMoveEvent& event)
{
    _inputs.handleMouseMove(event);

    _project->cameraMan().handleMouseMove(_inputs, event);
}

void Universe::handleMouseButton(const MouseButtonEvent& event)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
        return;

    _inputs.handleMouseButton(event);

    _project->cameraMan().handleMouseButton(_inputs, event);
}

void Universe::handleMouseScroll(const MouseScrollEvent& event)
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

    //_project.reset(new SolarSystemProject());
    _project.reset(new PathTracerProject());

    _project->addView(_viewport);

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
