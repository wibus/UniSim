layout (std140) uniform PhysicalSkyParams
{
    vec4 sunDirection;
    vec4 moonDirection;
    vec4 moonQuaternion;
    vec4 starsQuaternion;
    float sunToMoonRatio;
    float groundHeightKM;
    float starsExposure;
};

uniform sampler2D Moon;
uniform sampler2D Stars;


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
        sunDirection.xyz,
        transmittance);

    skyLuminance += sunToMoonRatio * GetSkyLuminance(
        camPosToEarthKM,
        viewDir,
        0,                // shadow_length
        moonDirection.xyz,
        transmittance);

    vec2 starsUv = findUV(starsQuaternion, viewDir);
    vec3 starsLuminance = texture2D(Stars, vec2(1 - starsUv.x, 1 - starsUv.y)).rgb;
    skyLuminance += transmittance * starsLuminance * starsExposure;
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
        sunDirection.xyz,
        transmittance);

    skyLuminance += sunToMoonRatio * GetSkyLuminanceToPoint(
        camPosToEarthKM,
        pointPosToEarthKM,
        0,                // shadow_length
        sunDirection.xyz,
        transmittance);
}

vec3 SampleDirectionalLightLuminance(vec3 viewDir, uint lightId)
{
    if(lightId == 0)
    {
        return directionalLights[lightId].emissionSolidAngle.rgb;
    }
    else if(lightId == 1)
    {
        vec3 offset = rotate(moonQuaternion, viewDir);
        double maxCos = 1 - directionalLights[lightId].emissionSolidAngle.a / (2 * PI);
        double maxSin = sqrt(1 - maxCos * maxCos);
        vec2 uv = (offset.xy / float(maxSin)) * 0.5 + 0.5;
        return texture(Moon, uv).rgb;
    }

    return vec3(0, 0, 0);
}
