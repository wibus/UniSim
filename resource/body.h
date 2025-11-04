#ifndef BODY_H
#define BODY_H

#include <GLM/glm.hpp>


namespace unisim
{

class Body
{
public:
    Body(double radius, double densityGramPerCm3, bool isStatic = false);
    virtual ~Body();

    bool isStatic() const { return _isStatic; }
    void setIsStatic(bool isStatic) { _isStatic = isStatic; }

    double mass() const { return _mass; }
    void setMass(double mass) { _mass = mass; }

    double area() const { return _area;}

    double volume() const { return _volume; }

    double density() const { return _mass / _volume; }

    glm::dvec3 position() const { return _position; }
    void setPosition(const glm::dvec3& position);

    glm::dvec3 linearVelocity() const { return _linearVelocity; }
    void setLinearVelocity(const glm::dvec3&  linearVelocity);

    glm::dvec4 quaternion() const { return _quaternion; }
    void setQuaternion(const glm::dvec4& quaternion);

    double angularSpeed() const { return _angularSpeed; }
    void setAngularSpeed(double angularSpeed);

    void setupOrbit(
            double a_semiMajorAxis,
            double e_eccentricity,
            double O_ascendingNode,
            double PI,
            double L_meanLongitude,
            double i_inclination,
            double secondsSinceJan1st2000,
            Body* parent);

    void setupRotation(
            double rotationPeriod,
            double phase,
            double rightAscension,
            double declination);

    void ui();

private:
    // transform
    glm::dvec4 _quaternion; // radians
    glm::dvec3 _position; // m

    // momentum
    double _angularSpeed; // radians / s
    glm::dvec3 _linearVelocity; // m / s

    // geometry
    double _area;
    double _volume;

    // composition
    double _mass; // Kg

    bool _isStatic;
};

}

#endif // BODY_H
