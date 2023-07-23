layout (std140, binding = 0) uniform CommonParams
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

layout (std140, binding = 1) buffer Instances
{
    Instance instances[];
};

layout (std140, binding = 2) buffer DirectionalLights
{
    DirectionalLight directionalLights[];
};

layout (std430, binding = 3) buffer Emitters
{
    uint emitters[];
};

layout (std140, binding = 4) buffer Materials
{
    Material materials[];
};

uniform layout(binding = 5, rgba32f) image2D result;
