#ifndef MESH_H
#define MESH_H



namespace unisim
{

class Mesh
{
public:
    Mesh(bool isSphere, float radius);
    virtual ~Mesh();

    bool isSphere() const { return _isSphere; }

    float radius() const { return _radius; }
    void setRadius(float radius) { _radius = radius; }

private:
    bool _isSphere;
    float _radius;
};

}

#endif // MESH_H
