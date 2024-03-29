#include "body.h"

#include <imgui/imgui.h>

#include "GLM/gtc/constants.hpp"
#include "GLM/gtx/transform.hpp"

#include "units.h"


namespace unisim
{

double solveEccentricAnomaly(double En, double e, double M, int depth)
{
    double En1 = En - (En - e*glm::sin(En) - M) / (1 - e*glm::cos(En));

    if(depth > 0)
        return solveEccentricAnomaly(En1, e, M, depth-1);
    else
        return En1;
}

double solveEccentricAnomaly(double e, double M)
{
    return solveEccentricAnomaly(M, e, M, 4);
}

Body::Body(double radius, double densityGramPerCm3, bool isStatic) :
    _quaternion(0, 0, 1, 0),
    _position(0, 0, 0),
    _angularSpeed(0),
    _linearVelocity(0, 0, 0),
    _area(4.0 * glm::pi<double>() * radius * radius),
    _volume(4.0 / 3.0 * glm::pi<double>() * radius * radius * radius),
    _mass(4 / 3.0f * glm::pi<double>() * radius * radius * radius * (densityGramPerCm3 * 1e3)),
    _isStatic(isStatic)
{

}

Body::~Body()
{

}

void Body::setPosition(const glm::dvec3 &position)
{
    _position = position;
}

void Body::setLinearVelocity(const glm::dvec3& linearVelocity)
{
    _linearVelocity = linearVelocity;
}

void Body::setQuaternion(const glm::dvec4& quaternion)
{
    _quaternion = quaternion;
}

void Body::setAngularSpeed(double angularSpeed)
{
    _angularSpeed = angularSpeed;
}

// ref: https://personal.math.ubc.ca/~cass/courses/m309-01a/orbits.pdf
void Body::setupOrbit(
        double a_semiMajorAxis,
        double e_eccentricity,
        double O_ascendingNode,
        double PI,
        double L_meanLongitude,
        double i_inclination,
        double secondsSinceJan1st2000,
        Body* parent)
{
    // AU to m
    a_semiMajorAxis = AU(a_semiMajorAxis);

    // Degrees to radians
    O_ascendingNode   = glm::radians(O_ascendingNode);
    PI                = glm::radians(PI);
    L_meanLongitude   = glm::radians(L_meanLongitude);
    i_inclination     = glm::radians(i_inclination);
    double w_argOfPerihelion = PI - O_ascendingNode;

    // Frame
    glm::dvec3 alpha(glm::cos(O_ascendingNode), glm::sin(O_ascendingNode), 0);
    glm::dvec3 w = glm::dmat3(glm::rotate(i_inclination, alpha)) * glm::dvec3(0, 0, 1);
    glm::dvec3 u = glm::dmat3(glm::rotate(w_argOfPerihelion, w)) * alpha;
    glm::dvec3 v = glm::cross(w, u);

    // Elipse
    double b = a_semiMajorAxis * glm::sqrt(1 - e_eccentricity*e_eccentricity);
    double f = a_semiMajorAxis * e_eccentricity;
    double aCube = a_semiMajorAxis*a_semiMajorAxis*a_semiMajorAxis;
    double P = 2 * glm::pi<double>() * glm::sqrt(aCube / (G * (_mass + parent->mass())));

    // Anomaly
    double T_0 = 0; // 1st Jan 2000 at midnight
    double M_0 = L_meanLongitude - PI;
    double T = T_0 - (M_0) / (2 * glm::pi<double>()) * P;

    double T_1 = T_0 + secondsSinceJan1st2000;
    double M_1 = 2 * glm::pi<double>() * (T_1 - T) / P;
    double E_1 = solveEccentricAnomaly(e_eccentricity, M_1);

    // Angular speed
    double T_2 = T_1 + 60*60; // +1h
    double M_2 = 2 * glm::pi<double>() * (T_2 - T) / P;
    double E_2 = solveEccentricAnomaly(e_eccentricity, M_2);

    // Coordinates
    double x_1 = a_semiMajorAxis * glm::cos(E_1) - f;
    double y_1 = b * glm::sin(E_1);

    // Velocity
    double x_2 = a_semiMajorAxis * glm::cos(E_2) - f;
    double y_2 = b * glm::sin(E_2);

    _position = x_1 * u + y_1 * v;
    _linearVelocity = ((x_2 * u + y_2 * v) - (x_1 * u + y_1 * v)) / (T_2 - T_1);

    if(parent)
    {
        _position +=       parent->position();
        _linearVelocity += parent->linearVelocity();
    }
}

void Body::setupRotation(
        double rotationPeriod,
        double phase,
        double rightAscension,
        double declination)
{
    _angularSpeed = 2 * glm::pi<double>() / ED(rotationPeriod);

    glm::dvec4 rightAscensionQuat = quat(glm::dvec3(0, 0, 1), glm::radians(rightAscension));
    glm::dvec4 declinationQuat    = quat(glm::dvec3(1, 0, 0), glm::radians(90 - declination));
    glm::dvec4 phaseQuat          = quat(glm::dvec3(0, 0, 1), glm::radians(phase));

    _quaternion = quatMul(EARTH_BASE_QUAT, quatMul(rightAscensionQuat, quatMul(declinationQuat, phaseQuat)));
}

void Body::ui()
{
    bool isStatic = _isStatic;
    if(ImGui::Checkbox("Is Static", &isStatic))
        setIsStatic(isStatic);

    double mass = _mass;
    if(ImGui::InputDouble("Mass", &mass, 0.1))
        setMass(glm::max(0.0, mass));

    double area = _area;
    ImGui::InputDouble("Area", &area, 0.0, 0.0, "%.6f", ImGuiInputTextFlags_ReadOnly);
    double volume = _volume;
    ImGui::InputDouble("Volume", &volume, 0.0, 0.0, "%.6f", ImGuiInputTextFlags_ReadOnly);
    double density = this->density();
    ImGui::InputDouble("Density", &density, 0.0, 0.0, "%.6f", ImGuiInputTextFlags_ReadOnly);

    glm::vec3 position = _position;
    if(ImGui::InputFloat3("Position", &position[0]))
        setPosition(glm::dvec3(position));

    glm::vec3 linearVelocity = _linearVelocity;
    if(ImGui::InputFloat3("Velocity", &linearVelocity[0]))
        setLinearVelocity(glm::dvec3(linearVelocity));

    glm::vec4 quaternion = _quaternion;
    if(ImGui::InputFloat4("Quaternion", &quaternion[0]))
        setQuaternion(glm::dvec4(quaternion));

    double angularSpeed = _angularSpeed;
    if(ImGui::InputDouble("Angular Speed", &angularSpeed, glm::pi<double>() / 180))
        setAngularSpeed(angularSpeed);
}

}
