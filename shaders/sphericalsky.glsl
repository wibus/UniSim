#version 440

#define PI 3.14159265359

uniform sampler2D skyMap;

uniform vec4 skyQuaternion;

uniform float skyExposure;


vec3 rotate(vec4 q, vec3 v);
vec2 findUV(vec4 quat, vec3 N);
vec3 toLinear(const vec3 sRGB);


void SampleSkyLuminance(
        out vec3 skyLuminance,
        out vec3 transmittance,
        vec3 cameraPos,
        vec3 viewDir)
{
    vec2 uv = findUV(skyQuaternion, viewDir);
    vec3 color = texture2D(skyMap, vec2(uv.x, 1 - uv.y)).rgb;

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
