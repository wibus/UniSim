#ifndef UNITS_H
#define UNITS_H

#include <type_traits>

#include <GLM/glm.hpp>
#include <GLM/gtx/quaternion.hpp>


inline double AU(double au) { return au * 1.495978707e11; }
inline double ED(double ed) { return ed * 24 * 60 * 60;   }

const double G = 6.67408e-11;

inline glm::dvec4 quat(const glm::dvec3& axis, double angle)
{
    double halfAngle = angle * 0.5;
    double cosHalfAngle = glm::cos(halfAngle);
    double sinHalfAngle = glm::sin(halfAngle);
    return glm::dvec4(axis * sinHalfAngle, cosHalfAngle);
}

inline glm::dvec4 quatMul(const glm::dvec4& q1, const glm::dvec4& q2)
{
    glm::dvec4 q;
    q.x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y);
    q.y = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x);
    q.z = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w);
    q.w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z);
    return q;
}

inline glm::dvec4 quatConjugate(const glm::dvec4& q)
{
    return glm::dvec4(-q.x, -q.y, -q.z, q.w);
}

inline glm::dvec4 quatInverse(const glm::dvec4& q)
{
    glm::dvec4 conjugate = quatConjugate(q);
    return conjugate / glm::length(conjugate);
}

inline glm::dvec3 rotatePoint(const glm::dvec4& q, const glm::dvec3& v)
{
    glm::dvec3 qxyz = glm::dvec3(q);
    return v + 2.0 * glm::cross(qxyz, glm::cross(qxyz, v) + q.w * v);
}

inline glm::mat3 quatMat3(const glm::dvec4& q)
{

    return glm::toMat3(glm::quat(q.w, q.x, q.y, q.z));
}

inline glm::mat4 quatMat4(const glm::dvec4& q)
{
    return glm::toMat4(glm::quat(q.w, q.x, q.y, q.z));
}

inline void makeOrthBase(const glm::vec3& N, glm::vec3& T, glm::vec3& B)
{
    T = glm::normalize(glm::cross(N, glm::abs(N.z) < 0.9 ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0)));
    B = glm::normalize(glm::cross(N, T));
}

const glm::dvec4 EARTH_BASE_QUAT = quat(glm::dvec3(1, 0, 0), glm::radians(-23.44));

#endif // UNITS_H
