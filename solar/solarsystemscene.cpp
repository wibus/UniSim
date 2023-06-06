#include "solarsystemscene.h"

#include "../units.h"
#include "../object.h"
#include "../body.h"
#include "../mesh.h"
#include "../material.h"


namespace unisim
{

std::shared_ptr<Object> makePlanet(const std::string& name, double radius, double density, Object* parent = nullptr)
{
    std::shared_ptr<Body> body(new Body(radius, density));
    std::shared_ptr<Mesh> mesh(new Mesh(true, radius));
    std::shared_ptr<Material> material(new Material(name));
    std::shared_ptr<Object> object(new Object(name, body, mesh, material, parent));
    return object;
}

SolarSystemScene::SolarSystemScene() :
    Scene("Solar System")
{
    _sky->setTexture("textures/background.jpg");
    _sky->setQuaternion(quatConjugate(EARTH_BASE_QUAT));

    // Sun
    std::shared_ptr<Object> sun = makePlanet("Sun", 696.340e6, 1.41f);
    sun->material()->setDefaultAlbedo(glm::dvec3(1.0, 1.0, 0.5));
    sun->material()->setDefaultEmissionColor(glm::dvec3(1.0, 1.0, 1.0) * 2.009e7);
    _objects.push_back(sun);

    // Planets
    std::shared_ptr<Object> mercury   = makePlanet("Mercury",   2.439e6,    5.43f,  &*sun);
    std::shared_ptr<Object> venus     = makePlanet("Venus",     6.051e6,    5.24f,  &*sun);
    std::shared_ptr<Object> earth     = makePlanet("Earth",     6.371e6,    5.51f,  &*sun);
    std::shared_ptr<Object> mars      = makePlanet("Mars",      3.389e6,    3.93f,  &*sun);
    std::shared_ptr<Object> jupiter   = makePlanet("Jupiter",   69.911e6,   1.33f,  &*sun);
    std::shared_ptr<Object> saturn    = makePlanet("Saturn",    58.232e6,   0.687f, &*sun);
    std::shared_ptr<Object> uranus    = makePlanet("Uranus",    25.362e6,   1.27f,  &*sun);
    std::shared_ptr<Object> neptune   = makePlanet("Neptune",   24.622e6,   1.64f,  &*sun);

    mercury->body()->setupOrbit(0.387,  0.206,   48.3,  77.46,  252.3,  7.00, &*sun->body());
    venus->body()->setupOrbit(  0.723,  0.007,   76.7,  131.6,  182.0,  3.39, &*sun->body());
    earth->body()->setupOrbit(  1.000,  0.017,    0.0,  102.9,  100.5,  0.00, &*sun->body());
    mars->body()->setupOrbit(   1.524,  0.093,   49.6,  336.1,  355.4,  1.85, &*sun->body());
    jupiter->body()->setupOrbit(5.203,  0.048,  100.4,   14.3,   34.4,  1.30, &*sun->body());
    saturn->body()->setupOrbit( 9.555,  0.056,  113.7,   93.1,   50.1,  2.49, &*sun->body());
    uranus->body()->setupOrbit( 19.22,  0.046,   74.0,  173.0,  314.1,  0.77, &*sun->body());
    neptune->body()->setupOrbit(30.11,  0.009,  131.8,   48.1,  304.3,  1.77, &*sun->body());

    sun->body()->setupRotation(    25.38, 0.0,  286.13,  63.87);
    mercury->body()->setupRotation( 58.6, 0.0,  281.01,  61.42);
    venus->body()->setupRotation( -243.0, 0.0,  272.76,  67.16);
    earth->body()->setupRotation(   0.99, 0.0,  000.00,  90.00);
    mars->body()->setupRotation(    1.03, 0.0,  317.68,  52.89);
    jupiter->body()->setupRotation( 0.41, 0.0,  268.05,  64.49);
    saturn->body()->setupRotation(  0.45, 0.0,   40.60,  83.54);
    uranus->body()->setupRotation( -0.72, 0.0,  257.43, -15.10);
    neptune->body()->setupRotation( 0.67, 0.0,  299.36,  43.46);

    auto setupMaterial = [](
            std::shared_ptr<Object> body,
            const std::string& name,
            const glm::vec3& defaultAlbedo,
            const glm::vec3& defaultEmission = glm::vec3(),
            float defaultEmissionLuminance = 0.0f)
    {
        std::shared_ptr<Material> material(new Material(name));
        material->loadAlbedo("textures/"+name+"_albedo.jpg");
        material->setDefaultAlbedo(defaultAlbedo);
        material->setDefaultEmissionColor(defaultEmission);
        material->setDefaultEmissionLuminance(defaultEmissionLuminance);
        body->setMaterial(material);
    };

    setupMaterial(sun,      "sun",      glm::vec3(1.0, 1.0, 0.5), glm::vec3(1.0, 1.0, 1.0), 2.009e7);
    setupMaterial(mercury,  "mercury",  glm::vec3(0.5, 0.5, 0.5));
    setupMaterial(venus,    "venus",    glm::vec3(0.8, 0.5, 0.2));
    setupMaterial(earth,    "earth",    glm::vec3(0.1, 0.8, 0.5));
    setupMaterial(mars,     "mars",     glm::vec3(0.8, 0.3, 0.1));
    setupMaterial(jupiter,  "jupiter",  glm::vec3(0.8, 0.6, 0.5));
    setupMaterial(saturn,   "saturn",   glm::vec3(0.8, 0.9, 0.3));
    setupMaterial(uranus,   "uranus",   glm::vec3(0.1, 0.5, 0.8));
    setupMaterial(neptune,  "neptune",  glm::vec3(0.1, 0.1, 0.7));

    _objects.push_back(mercury);
    _objects.push_back(venus);
    _objects.push_back(earth);
    _objects.push_back(mars);
    _objects.push_back(jupiter);
    _objects.push_back(saturn);
    _objects.push_back(uranus);
    _objects.push_back(neptune);

    // Moons
    std::shared_ptr<Object> moon = makePlanet("Moon", 1.738e6, 3.344, &*earth);

    moon->body()->setupOrbit(0.00257, 0.0554, 125.08, 83.23, 135.27, 5.16, &*earth->body());

    moon->body()->setupRotation(27.322, -45.0, 148.5, 73.0);

    setupMaterial(moon, "moon", glm::dvec3(0.5, 0.5, 0.5));

    _objects.push_back(moon);
}

}
