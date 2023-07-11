#version 440

uniform vec3 sunDirection;

uniform float groundHeightKM;


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
    skyLuminance = GetSkyLuminance(
        cameraPos * 1e-3 + vec3(0, 0, groundHeightKM), // to KM
        viewDir,
        0,                // shadow_length
        sunDirection,
        transmittance);
}


void SampleSkyLuminanceToPoint(
        out vec3 skyLuminance,
        out vec3 transmittance,
        vec3 cameraPos,
        vec3 pointPos)
{
    skyLuminance = GetSkyLuminanceToPoint(
        cameraPos * 1e-3 + vec3(0, 0, groundHeightKM), // to KM
        pointPos  * 1e-3, // to KM
        0,                // shadow_length
        sunDirection,
        transmittance);
}
