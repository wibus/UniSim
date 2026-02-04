#ifndef TERRAIN_H
#define TERRAIN_H

#include <memory>
#include <vector>


namespace unisim
{

class Instance;
class Material;
class Plane;


class Terrain
{
public:
    Terrain(double baseHeight, const std::shared_ptr<Material>& material, float uvScale = 1.0f);
    ~Terrain();

    const std::vector<std::shared_ptr<Instance>>& instances() const { return _instances; }

    void setMaterial(const std::shared_ptr<Material>& material);

    void ui();

private:
    std::shared_ptr<Plane> _plane;
    std::vector<std::shared_ptr<Instance>> _instances;
};

}

#endif // TERRAIN_H
