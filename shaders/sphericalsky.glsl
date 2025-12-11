layout (std140) uniform SkySphereParams
{
    vec4 skyQuaternion;
    float skyExposure;
};

uniform sampler2D Stars;


void SampleSkyLuminance(
        out vec3 skyLuminance,
        out vec3 transmittance,
        vec3 cameraPos,
        vec3 viewDir)
{
    vec2 uv = findUV(skyQuaternion, viewDir);
    vec3 color = texture2D(Stars, vec2(1 - uv.x, 1 - uv.y)).rgb;

    skyLuminance = color * skyExposure;

    transmittance = vec3(1, 1, 1);
}


void SampleSkyLuminanceToPoint(
        out vec3 skyLuminance,
        out vec3 transmittance,
        vec3 cameraPos,
        vec3 pointPos)
{
    skyLuminance = vec3(0, 0, 0);
    transmittance = vec3(1, 1, 1);
}

vec3 SampleDirectionalLightLuminance(vec3 viewDir, uint lightId)
{
    return vec3(0.0);
}
