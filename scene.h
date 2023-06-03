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

private:
    std::shared_ptr<Texture> _texture;
};


class DirectionalLight
{
public:
    DirectionalLight();
    ~DirectionalLight();

    glm::dvec3 position() const { return _position; }
    void setPosition(const glm::dvec3& position) { _position = position; }

    glm::dvec3 radiance() const { return _radiance; }
    void setRadiance(const glm::dvec3& radiance) { _radiance = radiance; }

    double solidAngle() const { return _solidAngle; }
    void setSolidAngle(double solidAngle) { _solidAngle = solidAngle; }

private:
    glm::dvec3 _position;
    glm::dvec3 _radiance;
    double _solidAngle;
};


class Scene
{
public:
    Scene();
    virtual ~Scene();

    std::shared_ptr<Sky> sky() const { return _sky; }

    std::vector<std::shared_ptr<Object>>& objects() { return _objects; }
    const std::vector<std::shared_ptr<Object>>& objects() const { return _objects; }

    std::vector<std::shared_ptr<DirectionalLight>>& directionalLights() { return _directionalLights; }
    const std::vector<std::shared_ptr<DirectionalLight>>& directionalLights() const { return _directionalLights; }

protected:
    std::shared_ptr<Sky> _sky;

    std::vector<std::shared_ptr<Object>> _objects;

    std::vector<std::shared_ptr<DirectionalLight>> _directionalLights;
};

}

#endif // SCENE_H
