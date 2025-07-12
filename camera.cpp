#include "camera.h"

#include <imgui/imgui.h>

#include "GLM/gtx/transform.hpp"


namespace unisim
{

Camera::Camera(Viewport viewport) :
    _viewport(viewport),
    _position(0, 0, 0),
    _lookAt(0, -1, 0),
    _up(0, 0, 1),
    _fov(45.0 / 180.0 * glm::pi<double>()),
    _ev(1),
    _filmHeight(0.024),
    _focalLength(0.035),
    _focusDistance(5.0f),
    _dofEnabled(true),
    _fstop(16),
    _shutterSpeed(1 / 100.0f),
    _iso(100)
{
    updateEV();
    // Update field of view
    setFocalLength(_focalLength);
}

void Camera::setViewport(Viewport viewport)
{
    _viewport = viewport;
}

void Camera::setFilmHeight(float height)
{
    _filmHeight = height;

    // Update focal length
    setFieldOfView(_fov);
}

void Camera::setFocalLength(float length)
{
    _focalLength = length;

    _fov = 2 * glm::atan(_filmHeight / (2 * _focalLength));
}

void Camera::setFieldOfView(float fov)
{
    _fov = fov;

    _focalLength = _filmHeight / glm::tan(_fov * 0.5f) * 0.5f;
}

void Camera::setFstop(float fstop)
{
    _fstop = fstop;
    updateEV();
}

void Camera::setShutterSpeed(float speed)
{
    _shutterSpeed = speed;
    updateEV();
}

void Camera::setIso(float iso)
{
    _iso = iso;
    updateEV();
}

void Camera::setEV(float ev)
{
    _ev = ev;

    _iso = 100.0f * _fstop * _fstop / (glm::exp2(_ev) * _shutterSpeed);
}

float Camera::exposure() const
{
    float sunIlluminanceNoon = 30000;
    float sunny16Light = (16 * 16 * 100);
    return (sunny16Light / glm::exp2(_ev)) / sunIlluminanceNoon;
}

void Camera::setPosition(const glm::dvec3& pos)
{
    _position = pos;
}

void Camera::setLookAt(const glm::dvec3& lookAt)
{
    _lookAt = lookAt;
}

void Camera::setUp(const glm::dvec3& up)
{
    _up = up;
}

glm::dmat4 Camera::view() const
{
   return glm::lookAt(
       glm::dvec3(_position.x, _position.y, _position.z),
       glm::dvec3(_lookAt.x,   _lookAt.y,   _lookAt.z),
       glm::dvec3(_up.x,       _up.y,       _up.z));
}

glm::mat4 Camera::proj() const
{
    return glm::perspective(
        _fov,
        _viewport.width / (float) _viewport.height,
        1.0f,
        2.0f);
}

glm::mat4 Camera::screen() const
{
    return
        glm::scale(glm::vec3(_viewport.width, _viewport.height, 1.0)) *
        glm::scale(glm::vec3(0.5, 0.5, 1.0)) *
        glm::translate(glm::vec3(1, 1, 0));
}

void Camera::updateEV()
{
    _ev = glm::log2(_fstop * _fstop * 100 / (_iso * _shutterSpeed));
}

void Camera::ui()
{
    int viewport[2] = {_viewport.width, _viewport.height};
    ImGui::InputInt2("Viewport", &viewport[0], ImGuiInputTextFlags_ReadOnly);

    float filmHeight = _filmHeight * 1000.0f;
    if(ImGui::InputFloat("Film Height mm", &filmHeight))
        setFilmHeight(filmHeight / 1000.0f);

    float focalLength = _focalLength * 1000.0f;
    if(ImGui::InputFloat("Focal Length", &focalLength))
        setFocalLength(focalLength / 1000.0f);

    float focusDistance = _focusDistance;
    if(ImGui::InputFloat("Focus Distance", &focusDistance))
        setFocusDistance(focusDistance);

    bool dofENabled = _dofEnabled;
    if(ImGui::Checkbox("DOF Enabled", &dofENabled))
        setDofEnabled(dofENabled);

    float fov = _fov;
    if(ImGui::SliderAngle("FOV", &fov, 1, 179))
        setFieldOfView(fov);

    float fstop = _fstop;
    if(ImGui::InputFloat("f-stop", &fstop, 0, 0, "%.1f"))
        setFstop(fstop);

    float shutterSpeed = _shutterSpeed;
    if(ImGui::InputFloat("Shutter Speed", &shutterSpeed))
        setShutterSpeed(shutterSpeed);

    float iso = _iso;
    if(ImGui::InputFloat("ISO", &iso))
        setIso(iso);

    float ev = _ev;
    if(ImGui::SliderFloat("EV", &ev, -6.0f, 17.0f))
        setEV(ev);

    ImGui::Separator();

    glm::vec3 position = _position;
    if(ImGui::InputFloat3("Position", &position[0]))
        setPosition(position);

    glm::vec3 lookAt = _lookAt;
    if(ImGui::InputFloat3("Look At", &lookAt[0]))
        setLookAt(lookAt);

    glm::vec3 up = _up;
    if(ImGui::InputFloat3("Up", &up[0]))
        setUp(glm::normalize(up));

    ImGui::Separator();
    glm::mat4 view = glm::transpose(this->view());
    ImGui::InputFloat4("View", &view[0][0]);
    ImGui::InputFloat4("",     &view[1][0]);
    ImGui::InputFloat4("",     &view[2][0]);
    ImGui::InputFloat4("",     &view[3][0]);

    ImGui::Separator();
    glm::mat4 proj = glm::transpose(this->proj());
    ImGui::InputFloat4("Projection", &proj[0][0]);
    ImGui::InputFloat4("",           &proj[1][0]);
    ImGui::InputFloat4("",           &proj[2][0]);
    ImGui::InputFloat4("",           &proj[3][0]);

    ImGui::Separator();
    glm::mat4 screen = glm::transpose(this->screen());
    ImGui::InputFloat4("Screen", &screen[0][0]);
    ImGui::InputFloat4("",       &screen[1][0]);
    ImGui::InputFloat4("",       &screen[2][0]);
    ImGui::InputFloat4("",       &screen[3][0]);

    ImGui::Separator();
}

CameraMan::CameraMan(Viewport viewport) :
    _camera(viewport)
{
}

CameraMan::~CameraMan()
{

}

void CameraMan::setViewport(Viewport viewport)
{
    _camera.setViewport(viewport);
}

void CameraMan::handleKeyboard(const Inputs& inputs, const KeyboardEvent& event)
{

}

void CameraMan::handleMouseMove(const Inputs& inputs, const MouseMoveEvent& event)
{

}

void CameraMan::handleMouseButton(const Inputs& inputs, const MouseButtonEvent& event)
{

}

void CameraMan::handleMouseScroll(const Inputs& inputs, const MouseScrollEvent& event)
{

}

}
