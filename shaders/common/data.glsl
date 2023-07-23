// Data structures
struct Instance
{
    vec4 albedo;
    vec4 emission;
    vec4 specular;
    vec4 position;
    vec4 quaternion;

    float radius;
    float mass;

    uint materialId;
    uint primitiveType;
};

struct DirectionalLight
{
    vec4 directionCosThetaMax;
    vec4 emissionSolidAngle;
};

struct Material
{
    layout(rgba8) readonly image2D albedo;
    layout(rgba8) readonly image2D specular;
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
    uint instanceId;
};

struct HitInfo
{
    uint instanceId;
    vec3 position;
    vec3 normal;
    vec3 diffuseAlbedo;
    vec3 specularF0;
    vec3 emission;
    float specularA;
    float specularA2;
    float NdotV;
    float instanceAreaPdf;
};
