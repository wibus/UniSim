layout (std140) uniform PathTracerCommonParams
{
    mat4 rayMatrix;
    vec4 lensePosition;
    vec4 lenseDirection;
    float focusDistance;
    float apertureRadius;
    float exposure;

    uint frameIndex;

    layout(rgba8) readonly image2D blueNoise[64];
    vec4 halton[64];
};

layout (std430) buffer Primitives
{
    Primitive primitives[];
};

layout (std430) buffer Meshes
{
    Mesh meshes[];
};

layout (std430) buffer Spheres
{
    Sphere spheres[];
};

layout (std430) buffer Planes
{
    Plane planes[];
};

layout (std430) buffer Instances
{
    Instance instances[];
};

layout (std430) buffer BvhNodes
{
    BvhNode bvhNodes[];
};

layout (std430) buffer Triangles
{
    Triangle triangles[];
};

layout (std430) buffer VerticesPos
{
    VertexPos verticesPos[];
};

layout (std430) buffer VerticesData
{
    VertexData verticesData[];
};

layout (std430) buffer Emitters
{
    Emitter emitters[];
};

layout (std430) buffer DirectionalLights
{
    DirectionalLight directionalLights[];
};

layout (std140) buffer Textures
{
    layout(rgba8) readonly image2D textures[];
};

layout (std430) buffer Materials
{
    Material materials[];
};

uniform layout(rgba32f) image2D result;
