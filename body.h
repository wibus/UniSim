#ifndef BODY_H
#define BODY_H

#include <GLM/glm.hpp>

#include <string>
#include <memory>

namespace unisim
{

class Material;

class Body
{
public:
    Body(const std::string& name, double radius, double density, Body* parent = nullptr);
    virtual ~Body();

    std::string name() const { return _name; }

    void setupOrbit(
            double a_semiMajorAxis,
            double e_eccentricity,
            double O_ascendingNode,
            double PI,
            double L_meanLongitude,
            double i_inclination);

    void setupRotation(
            double rotationPeriod,
            double phase,
            double rightAscension,
            double declination);

    glm::dvec3 albedo() const { return _albedo; }
    void setAlbedo(const glm::dvec3& albedo);

    glm::dvec3 emission() const { return _emission; }
    void setEmission(const glm::dvec3& emission);

    void setMaterial(const std::shared_ptr<Material>& material);
    std::shared_ptr<Material> material() const { return _material; }

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

private:
    Body* _parent;
    std::string _name;
    glm::dvec3 _albedo;
    glm::dvec3 _emission;
    std::shared_ptr<Material> _material;

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
};

}

#endif // BODY_H
