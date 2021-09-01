#include "universe.h"

#include "material.h"


namespace unisim
{

double g_MouseX = 0;
double g_MouseY = 0;
int g_MouseButton = -1;

bool g_KeyPressed[256];

double g_WindowWidth = 1280;
double g_WindowHeight = 720;

Universe& Universe::getInstance()
{
    static Universe g_Instance;

    return g_Instance;
}

void handleWindowResize(GLFWwindow* window, int width, int height)
{
    g_WindowWidth = width;
    g_WindowHeight = height;

    glViewport(0, 0, width, height);

    Universe::getInstance().cameraMan().setResolution(width, height);
}

void handleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    CameraMan& cameraMan = Universe::getInstance().cameraMan();

    if(action == GLFW_PRESS)
    {
        // Some keystrokes are handled in void Universe::update()
        g_KeyPressed[key] = true;

        if(GLFW_KEY_0 <= key && key <= GLFW_KEY_9)
        {
            cameraMan.setBodyIndex(key - GLFW_KEY_0);
        }
        else if(key == GLFW_KEY_I)
        {
            cameraMan.setMode(CameraMan::Mode::Static);
        }
        else if(key == GLFW_KEY_O)
        {
            cameraMan.setMode(CameraMan::Mode::Orbit);
        }
        else if(key == GLFW_KEY_P)
        {
            cameraMan.setMode(CameraMan::Mode::Ground);
        }
        else if(key == GLFW_KEY_KP_ADD)
        {
            cameraMan.exposeUp();
        }
        else if(key == GLFW_KEY_KP_SUBTRACT)
        {
            cameraMan.exposeDown();
        }
        else if(key == GLFW_KEY_SPACE)
        {
            if(Universe::getInstance().timeFactor() == 1.0)
                Universe::getInstance().setTimeFactor(0.0);
            else
                Universe::getInstance().setTimeFactor(1.0);
        }
    }
    else if(action == GLFW_RELEASE)
    {
        g_KeyPressed[key] = false;
    }
}

void handleMouseMove(GLFWwindow* window, double xpos, double ypos)
{
    int dx = xpos - g_MouseX;
    int dy = ypos - g_MouseY;

    g_MouseX = xpos;
    g_MouseY = ypos;

    if(g_MouseButton == GLFW_MOUSE_BUTTON_LEFT)
    {
        Universe::getInstance().cameraMan().rotatePrimary(dx, dy);
    }
    else if(g_MouseButton == GLFW_MOUSE_BUTTON_RIGHT)
    {
        Universe::getInstance().cameraMan().rotateSecondary(dx, dy);
    }
}

void handleMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT)
            g_MouseButton = GLFW_MOUSE_BUTTON_LEFT;
        else if(button == GLFW_MOUSE_BUTTON_RIGHT)
            g_MouseButton = GLFW_MOUSE_BUTTON_RIGHT;
    }
    else if(action == GLFW_RELEASE)
    {
        if(button == GLFW_MOUSE_BUTTON_LEFT)
            g_MouseButton = -1;
        else if(button == GLFW_MOUSE_BUTTON_RIGHT)
            g_MouseButton = -1;
    }
}

void handleScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if(yoffset != 0)
    {
        double cx = g_MouseX / g_WindowWidth;
        double cy = g_MouseY / g_WindowHeight;

        if(yoffset > 0)
        {
            Universe::getInstance().cameraMan().zoomIn();
        }
        else if(yoffset < 0)
        {
            Universe::getInstance().cameraMan().zoomOut();
        }
    }
}

Universe::Universe() :
    _timeFactor(0.0),
    _cameraMan(_camera, _bodies)
{

}

void Universe::setTimeFactor(double tf)
{
    _timeFactor = tf;
}

int Universe::launch(int argc, char** argv)
{
    // Copy because glut is taking a pointer
    _argc = argc;
    _argv = argv;

    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    window = glfwCreateWindow(g_WindowWidth, g_WindowHeight, "Universe", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    glfwSwapInterval(0);

    glViewport(0, 0, g_WindowWidth, g_WindowHeight);

    glfwSetWindowSizeCallback(window, handleWindowResize);
    glfwSetKeyCallback(window, handleKeyboard);
    glfwSetCursorPosCallback(window, handleMouseMove);
    glfwSetMouseButtonCallback(window, handleMouseButton);
    glfwSetScrollCallback(window, handleScroll);

    setup();

    _gravity.initialize(_bodies);
    _radiation.initialize(_bodies);

    _dt = 0;
    _lastTime = std::chrono::high_resolution_clock::now();

    _camera.setResolution(g_WindowWidth, g_WindowHeight);
    _camera.setExposure(0.004);
    _cameraMan.setBodyIndex(3);
    _cameraMan.setMode(CameraMan::Mode::Orbit);
    _cameraMan.setDistance(2.5e7);

    for(int i = 0; i < 256; ++i)
        g_KeyPressed[i] = false;

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

void Universe::setup()
{
    // Sun
    Body* sun = new Body("Sun", 696.340e6, 1.41f);
    sun->setAlbedo(glm::dvec3(1.0, 1.0, 0.5));
    sun->setEmission(glm::dvec3(1.0, 1.0, 1.0) * 2.009e7);
    _bodies.push_back(sun);

    // Planets
    Body* mercury   = new Body("Mercury",   2.439e6,    5.43f,  sun);
    Body* venus     = new Body("Venus",     6.051e6,    5.24f,  sun);
    Body* earth     = new Body("Earth",     6.371e6,    5.51f,  sun);
    Body* mars      = new Body("Mars",      3.389e6,    3.93f,  sun);
    Body* jupiter   = new Body("Jupiter",   69.911e6,   1.33f,  sun);
    Body* saturn    = new Body("Saturn",    58.232e6,   0.687f, sun);
    Body* uranus    = new Body("Uranus",    25.362e6,   1.27f,  sun);
    Body* neptune   = new Body("Neptune",   24.622e6,   1.64f,  sun);

    venus->setAlbedo(glm::dvec3(0.8, 0.5, 0.2));
    mercury->setAlbedo(glm::dvec3(0.5, 0.5, 0.5));
    earth->setAlbedo(glm::dvec3(0.1, 0.8, 0.5));
    mars->setAlbedo(glm::dvec3(0.8, 0.3, 0.1));
    jupiter->setAlbedo(glm::dvec3(0.8, 0.6, 0.5));
    saturn->setAlbedo(glm::dvec3(0.8, 0.9, 0.3));
    uranus->setAlbedo(glm::dvec3(0.1, 0.5, 0.8));
    neptune->setAlbedo(glm::dvec3(0.1, 0.1, 0.7));

    mercury->setupOrbit(0.387,  0.206,   48.3,  77.46,  252.3,  7.00);
    venus->setupOrbit(  0.723,  0.007,   76.7,  131.6,  182.0,  3.39);
    earth->setupOrbit(  1.000,  0.017,    0.0,  102.9,  100.5,  0.00);
    mars->setupOrbit(   1.524,  0.093,   49.6,  336.1,  355.4,  1.85);
    jupiter->setupOrbit(5.203,  0.048,  100.4,   14.3,   34.4,  1.30);
    saturn->setupOrbit( 9.555,  0.056,  113.7,   93.1,   50.1,  2.49);
    uranus->setupOrbit( 19.22,  0.046,   74.0,  173.0,  314.1,  0.77);
    neptune->setupOrbit(30.11,  0.009,  131.8,   48.1,  304.3,  1.77);

    sun->setupRotation(    25.38, 0.0,  286.13,  63.87);
    mercury->setupRotation( 58.6, 0.0,  281.01,  61.42);
    venus->setupRotation( -243.0, 0.0,  272.76,  67.16);
    earth->setupRotation(   0.99, 0.0,  000.00,  90.00);
    mars->setupRotation(    1.03, 0.0,  317.68,  52.89);
    jupiter->setupRotation( 0.41, 0.0,  268.05,  64.49);
    saturn->setupRotation(  0.45, 0.0,   40.60,  83.54);
    uranus->setupRotation( -0.72, 0.0,  257.43, -15.10);
    neptune->setupRotation( 0.67, 0.0,  299.36,  43.46);

    auto setupMaterial = [](Body* body, const std::string& name)
    {
        std::shared_ptr<Material> material(new Material(name));
        material->loadAlbedo("textures/"+name+"_albedo.jpg");
        body->setMaterial(material);
    };

    setupMaterial(sun,      "sun");
    setupMaterial(mercury,  "mercury");
    setupMaterial(venus,    "venus");
    setupMaterial(earth,    "earth");
    setupMaterial(mars,     "mars");
    setupMaterial(jupiter,  "jupiter");
    setupMaterial(saturn,   "saturn");
    setupMaterial(uranus,   "uranus");
    setupMaterial(neptune,  "neptune");

    _bodies.push_back(mercury);
    _bodies.push_back(venus);
    _bodies.push_back(earth);
    _bodies.push_back(mars);
    _bodies.push_back(jupiter);
    _bodies.push_back(saturn);
    _bodies.push_back(uranus);
    _bodies.push_back(neptune);

    // Moons
    Body* moon = new Body("Moon", 1.738e6, 3.344, earth);

    moon->setAlbedo(glm::dvec3(0.5, 0.5, 0.5));

    moon->setupOrbit(0.00257, 0.0554, 125.08, 83.23, 135.27, 5.16);

    moon->setupRotation(27.322, -45.0, 266.86, 65.64);

    setupMaterial(moon, "moon");

    _bodies.push_back(moon);
}

void Universe::update()
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeDiff = currentTime - _lastTime;
    _lastTime = currentTime;
    _dt = timeDiff.count();

    _dt *= 60 * 60 * _timeFactor;

    if(g_KeyPressed[GLFW_KEY_W])
        _cameraMan.moveForward();
    if(g_KeyPressed[GLFW_KEY_S])
        _cameraMan.moveBackward();
    if(g_KeyPressed[GLFW_KEY_A])
        _cameraMan.strafeLeft();
    if(g_KeyPressed[GLFW_KEY_D])
        _cameraMan.strafeRight();
    if(g_KeyPressed[GLFW_KEY_E])
        _cameraMan.strafeUp();
    if(g_KeyPressed[GLFW_KEY_Q])
        _cameraMan.strafeDown();

    _gravity.update(_bodies, _dt);
    _cameraMan.update(_dt);
}

void Universe::draw()
{
    _radiation.draw(_bodies, _dt, _camera);
}

}
