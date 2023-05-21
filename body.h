#ifndef BODY_H
#define BODY_H

#include <GLM/glm.hpp>


namespace unisim
{

class Body
{
public:
    Body(double radius, double density, bool isStatic = false);
    virtual ~Body();

    void setupOrbit(
            double a_semiMajorAxis,
            double e_eccentricity,
            double O_ascendingNode,
            double PI,
            double L_meanLongitude,
            double i_inclination,
            Body* parent);

    void setupRotation(
            double rotationPeriod,
            double phase,
            double rightAscension,
            double declination);

    glm::dvec4 quaternion() const { return _quaternion; }
    void setQuaternion(const glm::dvec4& quaternion);

    glm::dvec3 position() const { return _position; }
    void setPosition(const glm::dvec3& position);

    double rotation() const { return _rotation; }
    void setRotation(double rotation);

    glm::dvec3 linearMomentum() const { return _linearMomentum; }
    void setLinearMomentum(const glm::dvec3&  momentum);

    double radius() const { return _radius; }

    double mass() const { return _mass; }

    bool isStatic() const { return _isStatic; }

private:
    // transform
    glm::dvec4 _quaternion; // radians
    glm::dvec3 _position; // m

    // momentum
    double _rotation; // radians / s
    glm::dvec3 _linearMomentum; // m / s

    // geometry
    double _radius; // m

    // composition
    double _mass; // Kg

    bool _isStatic;
};

}

#endif // BODY_H
