#version 440

#define PI 3.14159265359

uniform vec3 sunDirection;
uniform vec3 moonDirection;
uniform float sunToMoonRatio;

uniform float groundHeightKM;

uniform sampler2D moonLighting;
uniform mat4 moonInvTransform;


struct DirectionalLight
{
    vec4 directionCosThetaMax;
    vec4 emissionSolidAngle;
};

layout (std140, binding = 2) buffer DirectionalLights
{
    DirectionalLight directionalLights[];
};

// Returns the luminance of the Sun, outside the atmosphere.
vec3 GetSolarLuminance();

// Returns the sky luminance along the segment from 'camera' to the nearest
// atmosphere boundary in direction 'view_ray', as well as the transmittance
// along this segment.
vec3 GetSkyLuminance(
        vec3 camera,
        vec3 view_ray,
        float shadow_length,
        vec3 sun_direction,
        out vec3 transmittance);

// Returns the sky luminance along the segment from 'camera' to 'p', as well as
// the transmittance along this segment.
vec3 GetSkyLuminanceToPoint(
        vec3 camera,
        vec3 p,
        float shadow_length,
        vec3 sun_direction,
        out vec3 transmittance);

// Returns the sun and sky illuminance received on a surface patch located at
// 'p' and whose normal vector is 'normal'.
vec3 GetSunAndSkyIlluminance(
        vec3 p,
        vec3 normal,
        vec3 sun_direction,
        out vec3 sky_illuminance);


// UniSim API

void SampleSkyLuminance(
        out vec3 skyLuminance,
        out vec3 transmittance,
        vec3 cameraPos,
        vec3 viewDir)
{
    vec3 camPosToEarthKM = cameraPos * 1e-3 + vec3(0, 0, groundHeightKM);

    skyLuminance = GetSkyLuminance(
        camPosToEarthKM,
        viewDir,
        0,                // shadow_length
        sunDirection,
        transmittance);

    skyLuminance += sunToMoonRatio * GetSkyLuminance(
        camPosToEarthKM,
        viewDir,
        0,                // shadow_length
        moonDirection,
        transmittance);
}


void SampleSkyLuminanceToPoint(
        out vec3 skyLuminance,
        out vec3 transmittance,
        vec3 cameraPos,
        vec3 pointPos)
{
    vec3 camPosToEarthKM = cameraPos * 1e-3 + vec3(0, 0, groundHeightKM);
    vec3 pointPosToEarthKM = pointPos * 1e-3 + vec3(0, 0, groundHeightKM);

    skyLuminance = GetSkyLuminanceToPoint(
        camPosToEarthKM,
        pointPosToEarthKM,
        0,                // shadow_length
        sunDirection,
        transmittance);

    skyLuminance += sunToMoonRatio * GetSkyLuminanceToPoint(
        camPosToEarthKM,
        pointPosToEarthKM,
        0,                // shadow_length
        sunDirection,
        transmittance);
}

vec3 SampleDirectionalLight(vec3 viewDir, uint lightId)
{
    if(lightId == 0)
        return directionalLights[lightId].emissionSolidAngle.rgb;
    else if(lightId == 1)
    {
        vec3 offset = (moonInvTransform * vec4(viewDir, 0)).xyz;
        double maxCos = 1 - directionalLights[lightId].emissionSolidAngle.a / (2 * PI);
        double maxSin = sqrt(1 - maxCos * maxCos);
        vec2 uv = (offset.xy / float(maxSin)) * 0.5 + 0.5;
        return texture(moonLighting, uv).rgb;
    }

    return vec3(0, 0, 0);
}
