// Data structures
struct Primitive
{
    uint type;
    uint index;
    uint material;
    uint pad1;
};

struct Mesh
{
    uint vertexStart;
    uint vertexEnd;
    uint triangleStart;
    uint triangleEnd;
    uint submeshStart;
    uint submeshEnd;
};

struct Sphere
{
    float radius;
};

struct Plane
{
    float invScale;
};

struct Instance
{
    vec4 position;
    vec4 quaternion;

    uint primitiveBegin;
    uint primitiveEnd;

    int pad1;
    int pad2;
};

struct Emitter
{
    uint instance;
    uint primitive;
};

struct DirectionalLight
{
    vec4 directionCosThetaMax;
    vec4 emissionSolidAngle;
};

struct Material
{
    vec4 albedo;
    vec4 emission;
    vec4 specular;

    int albedoTexture;
    int specularTexture;
    int pad1;
    int pad2;
};

struct Ray
{
    vec3 origin;
    vec3 direction;
    vec3 throughput;
#ifndef UNBIASED
    float diffusivity;
#endif
    uint depth;
    float bsdfPdf;
};

struct Intersection
{
    float t;
    uint materialId;
    vec3 normal;
    vec2 uv;
    float primitiveAreaPdf;
};

struct HitInfo
{
    vec3 position;
    vec3 normal;
    vec3 diffuseAlbedo;
    vec3 specularF0;
    vec3 emission;
    float specularA;
    float specularA2;
    float NdotV;
    float primitiveAreaPdf;
};

struct LightSample
{
    vec3 direction;
    float distance;
    vec3 emission;
    float solidAngle;
};
