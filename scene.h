#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>
#include <memory>

#include <GLM/glm.hpp>


namespace unisim
{

class Object;
class Texture;


class Sky
{
public:
    Sky();

    std::shared_ptr<Texture> texture() const { return _texture; }
    void setTexture(const std::shared_ptr<Texture>& texture) { _texture = texture; }
    void setTexture(const std::string& fileName);

    glm::vec4 quaternion() const { return _quaternion; }
    void setQuaternion(const glm::vec4& quaternion) { _quaternion = quaternion; }

    float exposure() const { return _exposure; }
    void setExposure(float exposure) { _exposure = exposure; }

private:
    std::shared_ptr<Texture> _texture;
    glm::vec4 _quaternion;
    float _exposure;
};


class DirectionalLight
{
public:
    DirectionalLight(const std::string& name);
    ~DirectionalLight();

    const std::string& name() const { return _name; }

    glm::vec3 direction() const { return _position; }
    void setDirection(const glm::vec3& position) { _position = position; }

    glm::vec3 emissionColor() const { return _emissionColor; }
    void setEmissionColor(const glm::vec3& color) { _emissionColor = color; }

    float emissionLuminance() const { return _emissionLuminance; }
    void setEmissionLuminance(float luminance) { _emissionLuminance = luminance; }

    float solidAngle() const { return _solidAngle; }
    void setSolidAngle(float solidAngle) { _solidAngle = solidAngle; }

private:
    std::string _name;
    glm::vec3 _position;
    glm::vec3 _emissionColor;
    float _emissionLuminance;
    float _solidAngle;
};


class Scene
{
public:
    Scene(const std::string& name);
    virtual ~Scene();

    const std::string& name() const { return _name; }

    std::shared_ptr<Sky> sky() const { return _sky; }

    std::vector<std::shared_ptr<Object>>& objects() { return _objects; }
    const std::vector<std::shared_ptr<Object>>& objects() const { return _objects; }

    std::vector<std::shared_ptr<DirectionalLight>>& directionalLights() { return _directionalLights; }
    const std::vector<std::shared_ptr<DirectionalLight>>& directionalLights() const { return _directionalLights; }

protected:
    std::string _name;

    std::shared_ptr<Sky> _sky;

    std::vector<std::shared_ptr<Object>> _objects;

    std::vector<std::shared_ptr<DirectionalLight>> _directionalLights;
};

}

#endif // SCENE_H
