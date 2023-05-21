#ifndef MESH_H
#define MESH_H



namespace unisim
{

class Mesh
{
public:
    Mesh(bool isSphere, double radius);
    virtual ~Mesh();

    bool isSphere() const { return _isSphere; }

    double radius() const { return _radius; }

private:
    bool _isSphere;
    double _radius;
};

}

#endif // MESH_H
