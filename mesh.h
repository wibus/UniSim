#ifndef MESH_H
#define MESH_H



namespace unisim
{

enum PrimitiveType
{
    Sphere = 0,
    Terrain = 1,
    Count = 2
};

extern const char* PrimitiveType_Names[PrimitiveType::Count];


class Mesh
{
public:
    Mesh(PrimitiveType primitiveType, float radius);
    virtual ~Mesh();

    PrimitiveType primitiveType() const { return _primitiveType; }

    float radius() const { return _radius; }
    void setRadius(float radius) { _radius = radius; }

private:
    PrimitiveType _primitiveType;
    float _radius;
};

}

#endif // MESH_H
