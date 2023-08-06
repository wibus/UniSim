#include "ui.h"

#include <imgui/imgui.h>

#include <GLM/gtc/constants.hpp>

#include "scene.h"
#include "object.h"
#include "body.h"
#include "primitive.h"
#include "material.h"
#include "camera.h"
#include "profiler.h"
#include "sky.h"
#include "graphic.h"


namespace unisim
{

Ui::Ui(bool showUi) :
    _showUi(showUi)
{

}

Ui::~Ui()
{

}


void Ui::show()
{
    _showUi = true;
}

void Ui::hide()
{
    _showUi = false;
}

bool displayExposure(float& exposure)
{
    float ev = glm::log2(exposure);
    if(ImGui::SliderFloat("Exposure", &ev, -20, 20))
    {
        exposure = glm::pow(2.0f, ev);
        return true;
    }

    return false;
}

void displayTexture(Texture* texture)
{
    if(texture == nullptr)
        return;

    int dimensions[2] = {texture->width, texture->height};
    ImGui::InputInt2("Dimensions", &dimensions[0], ImGuiInputTextFlags_ReadOnly);
    ImGui::Text("Format %s", texture->format == Texture::UNORM8 ? "UNORM8" : "Float32");
    int numComponents = texture->numComponents;
    ImGui::InputInt("Num Components", &numComponents);
    uint64_t handle = texture->handle;
    ImGui::Image((void*)handle, ImVec2(512, (512.0f / dimensions[0]) * dimensions[1]));
}

void displayCamera(Camera& camera)
{
    int viewport[2] = {camera.viewport().width, camera.viewport().height};
    ImGui::InputInt2("Viewport", &viewport[0], ImGuiInputTextFlags_ReadOnly);

    float filmHeight = camera.filmHeight() * 1000.0f;
    if(ImGui::InputFloat("Film Height mm", &filmHeight))
        camera.setFilmHeight(filmHeight / 1000.0f);

    float focalLength = camera.focalLength() * 1000.0f;
    if(ImGui::InputFloat("Focal Length", &focalLength))
        camera.setFocalLength(focalLength / 1000.0f);

    float focusDistance = camera.focusDistance();
    if(ImGui::InputFloat("Focus Distance", &focusDistance))
        camera.setFocusDistance(focusDistance);

    bool dofENabled = camera.dofEnable();
    if(ImGui::Checkbox("DOF Enabled", &dofENabled))
        camera.setDofEnabled(dofENabled);

    float fov = camera.fieldOfView();
    if(ImGui::SliderAngle("FOV", &fov, 1, 179))
        camera.setFieldOfView(fov);

    float fstop = camera.fstop();
    if(ImGui::InputFloat("f-stop", &fstop, 0, 0, "%.1f"))
        camera.setFstop(fstop);

    float shutterSpeed = camera.shutterSpeed();
    if(ImGui::InputFloat("Shutter Speed", &shutterSpeed))
        camera.setShutterSpeed(shutterSpeed);

    float iso = camera.iso();
    if(ImGui::InputFloat("ISO", &iso))
        camera.setIso(iso);

    float ev = camera.ev();
    if(ImGui::SliderFloat("EV", &ev, -6.0f, 17.0f))
        camera.setEV(ev);

    ImGui::Separator();

    glm::vec3 position = camera.position();
    if(ImGui::InputFloat3("Position", &position[0]))
        camera.setPosition(position);

    glm::vec3 lookAt = camera.lookAt();
    if(ImGui::InputFloat3("Look At", &lookAt[0]))
        camera.setLookAt(lookAt);

    glm::vec3 up = camera.up();
    if(ImGui::InputFloat3("Up", &up[0]))
        camera.setUp(glm::normalize(up));

    ImGui::Separator();
    glm::mat4 view = glm::transpose(camera.view());
    ImGui::InputFloat4("View", &view[0][0]);
    ImGui::InputFloat4("",     &view[1][0]);
    ImGui::InputFloat4("",     &view[2][0]);
    ImGui::InputFloat4("",     &view[3][0]);

    ImGui::Separator();
    glm::mat4 proj = glm::transpose(camera.proj());
    ImGui::InputFloat4("Projection", &proj[0][0]);
    ImGui::InputFloat4("",           &proj[1][0]);
    ImGui::InputFloat4("",           &proj[2][0]);
    ImGui::InputFloat4("",           &proj[3][0]);

    ImGui::Separator();
    glm::mat4 screen = glm::transpose(camera.screen());
    ImGui::InputFloat4("Screen", &screen[0][0]);
    ImGui::InputFloat4("",       &screen[1][0]);
    ImGui::InputFloat4("",       &screen[2][0]);
    ImGui::InputFloat4("",       &screen[3][0]);

    ImGui::Separator();
}

void displaySky(const ResourceManager& resources, Scene& scene)
{
    if(ImGui::TreeNode("Sky"))
    {
        glm::vec4 quaternion = scene.sky()->quaternion();
        if(ImGui::InputFloat4("Quaternion", &quaternion[0]))
        {
            quaternion = glm::normalize(quaternion);
            scene.sky()->setQuaternion(quaternion);
        }

        float exposure = scene.sky()->exposure();
        if(displayExposure(exposure))
        {
            scene.sky()->setExposure(exposure);
        }

        SkyLocalization& local = scene.sky()->localization();

        glm::vec2 geom(local.longitude(), local.latitude());
        if(ImGui::InputFloat2("Long/Lat", &geom[0]))
        {
            local.setLongitude(geom[0]);
            local.setLatitude(geom[1]);
        }

        float timeOfDay = local.timeOfDay();
        if(ImGui::SliderFloat("Time of Day", &timeOfDay, 0, SkyLocalization::MAX_TIME_OF_DAY))
            local.setTimeOfDay(timeOfDay);

        float dayOfYear = local.dayOfYear();
        if(ImGui::SliderFloat("Day of Year", &dayOfYear, 0, SkyLocalization::MAX_DAY_OF_YEAR))
            local.setDayOfYear(dayOfYear);

        float dayOfMoon = local.dayOfMoon();
        if(ImGui::SliderFloat("Day of Moon", &dayOfMoon, 0, SkyLocalization::MAX_DAY_OF_MOON))
            local.setDayOfMoon(dayOfMoon);

        ImGui::TreePop();
    }
}

void displayDirectionalLights(Scene& scene)
{
    if(ImGui::TreeNode("Directional Lights"))
    {
        for(const auto& light : scene.sky()->directionalLights())
        {
            if(ImGui::TreeNode(light->name().c_str()))
            {
                glm::vec3 direction = light->direction();
                if(ImGui::SliderFloat3("Direction", &direction[0], -1, 1))
                    light->setDirection(glm::normalize(direction));

                glm::vec3 emissionColor = light->emissionColor();
                if(ImGui::ColorPicker3("Emission Color", &emissionColor[0]))
                    light->setEmissionColor(emissionColor);

                float emissionLuminance = light->emissionLuminance();
                if(ImGui::InputFloat("Emission Luminance", &emissionLuminance))
                    light->setEmissionLuminance(glm::max(0.0f, emissionLuminance));

                float solidAngle = light->solidAngle();
                if(ImGui::InputFloat("Solid Angle", &solidAngle, 0.0f, 0.0f, "%.6f"))
                    light->setSolidAngle(glm::clamp(solidAngle, 0.0f ,4 * glm::pi<float>()));

                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }
}

void displayObjects(Scene& scene)
{
    if(ImGui::TreeNode("Objects"))
    {
        for(const auto& object : scene.objects())
        {
            if(ImGui::TreeNode(object->name().c_str()))
            {
                auto body = object->body();

                if(body && ImGui::TreeNode("Body"))
                {
                    bool isStatic = body->isStatic();
                    if(ImGui::Checkbox("Is Static", &isStatic))
                        body->setIsStatic(isStatic);

                    double mass = body->mass();
                    if(ImGui::InputDouble("Mass", &mass, 0.1))
                        body->setMass(glm::max(0.0, mass));

                    double area = body->area();
                    ImGui::InputDouble("Area", &area, 0.0, 0.0, "%.6f", ImGuiInputTextFlags_ReadOnly);
                    double volume = body->volume();
                    ImGui::InputDouble("Volume", &volume, 0.0, 0.0, "%.6f", ImGuiInputTextFlags_ReadOnly);
                    double density = body->density();
                    ImGui::InputDouble("Density", &density, 0.0, 0.0, "%.6f", ImGuiInputTextFlags_ReadOnly);

                    glm::vec3 position = body->position();
                    if(ImGui::InputFloat3("Position", &position[0]))
                        body->setPosition(glm::dvec3(position));

                    glm::vec3 linearVelocity = body->linearVelocity();
                    if(ImGui::InputFloat3("Velocity", &linearVelocity[0]))
                        body->setLinearVelocity(glm::dvec3(linearVelocity));

                    glm::vec4 quaternion = body->quaternion();
                    if(ImGui::InputFloat4("Quaternion", &quaternion[0]))
                        body->setQuaternion(glm::dvec4(quaternion));

                    double angularSpeed = body->angularSpeed();
                    if(ImGui::InputDouble("Angular Speed", &angularSpeed, glm::pi<double>() / 180))
                        body->setAngularSpeed(angularSpeed);

                    ImGui::TreePop();
                }

                if(!object->primitives().empty() && ImGui::TreeNode("Primitives"))
                {
                    for(std::size_t p = 0; p < object->primitives().size(); ++p)
                    {
                        Primitive& primitive = *object->primitives()[p];

                        if(ImGui::TreeNode(&primitive, "Primitive %u: %s", (uint)p, Primitive::Type_Names[primitive.type()]))
                        {
                            switch(primitive.type())
                            {
                            case Primitive::Mesh :
                                {
                                    Mesh& mesh = static_cast<Mesh&>(primitive);
                                    ImGui::Text("Vertex count: %u", (uint)mesh.vertices().size());
                                    ImGui::Text("Triangle count: %u", (uint)mesh.triangles().size());
                                    ImGui::Text("Sub-mesh count: %u", (uint)mesh.subMeshes().size());
                                }
                                break;
                            case Primitive::Sphere :
                                {
                                    Sphere& sphere = static_cast<Sphere&>(primitive);
                                    float radius = sphere.radius();
                                    if(ImGui::InputFloat("Radius", &radius))
                                        sphere.setRadius(radius);
                                }
                                break;
                            case Primitive::Plane :
                                {
                                    Plane& plane = static_cast<Plane&>(primitive);
                                }
                                break;
                            default:
                                assert(false);
                            }

                            Material* material = primitive.material().get();
                            if(material && ImGui::TreeNode("Material"))
                            {
                                displayTexture(material->albedo());

                                glm::vec3 albedo = material->defaultAlbedo();
                                if(ImGui::ColorPicker3("Albedo", &albedo[0]))
                                    material->setDefaultAlbedo(albedo);

                                glm::vec3 emissionColor = material->defaultEmissionColor();
                                if(ImGui::ColorPicker3("Emission", &emissionColor[0]))
                                    material->setDefaultEmissionColor(emissionColor);

                                float emissionLuminance = material->defaultEmissionLuminance();
                                if(ImGui::InputFloat("Luminance", &emissionLuminance))
                                    material->setDefaultEmissionLuminance(emissionLuminance);

                                float roughness = material->defaultRoughness();
                                if(ImGui::SliderFloat("Roughness", &roughness, 0, 1))
                                    material->setDefaultRoughness(roughness);

                                float metalness = material->defaultMetalness();
                                if(ImGui::SliderFloat("Metalness", &metalness, 0, 1))
                                    material->setDefaultMetalness(metalness);

                                float reflectance = material->defaultReflectance();
                                if(ImGui::InputFloat("Reflectance", &reflectance, 0.01f))
                                {
                                    reflectance = glm::clamp(reflectance, 0.0f, 1.0f);
                                    material->setDefaultReflectance(reflectance);
                                }

                                ImGui::TreePop();
                            }

                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }
}

void Ui::render(const ResourceManager& resources, Scene &scene, Camera& camera)
{
    if(!_showUi)
        return;

    if(ImGui::Begin(scene.name().c_str(), &_showUi))
    {
        if(ImGui::BeginTabBar("#tabs"))
        {
            if(ImGui::BeginTabItem("Camera"))
            {
                displayCamera(camera);
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Scene"))
            {
                displaySky(resources, scene);
                displayDirectionalLights(scene);
                displayObjects(scene);
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Profiler"))
            {
                Profiler::GetInstance().renderUi();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();
}

}
