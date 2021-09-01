#ifndef UNITS_H
#define UNITS_H

#include "GLM/glm.hpp"

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

inline glm::vec3 rotatePoint(const glm::dvec4& q, const glm::dvec3& v)
{
    glm::dvec3 qxyz = glm::dvec3(q);
    return v + 2.0 * glm::cross(qxyz, glm::cross(qxyz, v) + q.w * v);
}

const glm::dvec4 EARTH_BASE_QUAT = quat(glm::dvec3(1, 0, 0), glm::radians(-23.44));

#endif // UNITS_H
