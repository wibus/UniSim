
#if defined(API_OPENGL)
layout (set=0, binding=0, std140) buffer BindlessTextures
{
    layout(rgba8) readonly image2D bindlessTextures[];
};
#elif defined(API_VULKAN)
layout (set=0, binding=0) uniform sampler2D bindlessTextures[];
#endif // defined(API_OPENGL)



layout (set=2, binding=13, std140) uniform PathTracerCommonParams
{
    mat4 rayMatrix;
    vec4 lensePosition;
    vec4 lenseDirection;
    float focusDistance;
    float apertureRadius;
    float exposure;

    uint frameIndex;

    int blueNoiseTextures[64];
    vec4 halton[64];
};

layout (set=2, binding=1, std430) buffer Primitives
{
    Primitive primitives[];
};

layout (set=2, binding=2, std430) buffer Meshes
{
    Mesh meshes[];
};

layout (set=2, binding=3, std430) buffer Spheres
{
    Sphere spheres[];
};

layout (set=2, binding=4, std430) buffer Planes
{
    Plane planes[];
};

layout (set=2, binding=5, std430) buffer Instances
{
    Instance instances[];
};

layout (set=2, binding=6, std430) buffer BvhNodes
{
    BvhNode bvhNodes[];
};

layout (set=2, binding=7, std430) buffer Triangles
{
    Triangle triangles[];
};

layout (set=2, binding=8, std430) buffer VerticesPos
{
    VertexPos verticesPos[];
};

layout (set=2, binding=9, std430) buffer VerticesData
{
    VertexData verticesData[];
};

layout (set=2, binding=10, std430) buffer Emitters
{
    Emitter emitters[];
};

layout (set=2, binding=11, std430) buffer DirectionalLights
{
    DirectionalLight directionalLights[];
};

layout (set=2, binding=12, std430) buffer Materials
{
    Material materials[];
};
