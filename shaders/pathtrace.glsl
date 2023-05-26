#version 440
#extension GL_ARB_bindless_texture : require

#define PI 3.14159265359
#define GOLDEN_RATIO 1.618033988749894
#define DELTA (1 / 0.0)

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

    float pad1;
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
    float roughness;
    float metalness;
    float reflectance;
    float areaPdf;
};

layout (std140, binding = 0) uniform CommonParams
{
    mat4 rayMatrix;
    vec4 eyePosition;
    uint instanceCount;
    float radiusScale;
    float exposure;
    uint emitterStart;
    uint emitterEnd;
    uint frameIndex;

    uint pad1;
    uint pad2;

    // Background
    vec4 backgroundQuat;
    layout(rgba32f) readonly image2D backgroundImg;

    layout(rgba8) readonly image2D blueNoise[64];

    vec2 halton[64];
};

layout (std140, binding = 1) uniform Instances
{
    Instance instances[100];
};

layout (std140, binding = 2) uniform Materials
{
    Material materials[100];
};

uniform layout(binding = 3, rgba32f) image2D result;

const uint PATH_LENGTH = 4;

float sqr(float v)
{
    return v * v;
}

float distanceSqr(vec3 a, vec3 b)
{
    vec3 diff = b - a;
    return dot(diff, diff);
}

float sRGB(float x)
{
    if (x <= 0.00031308)
        return 12.92 * x;
    else
        return 1.055*pow(x,(1.0 / 2.4) ) - 0.055;
}

vec3 sRGB(vec3 c)
{
    return vec3(sRGB(c.x),sRGB(c.y),sRGB(c.z));
}

vec3 toLinear(const vec3 sRGB)
{
    bvec3 cutoff = lessThan(sRGB, vec3(0.04045));
    vec3 higher = pow((sRGB + vec3(0.055))/vec3(1.055), vec3(2.4));
    vec3 lower = sRGB/vec3(12.92);

    return mix(higher, lower, cutoff);
}

float toLuminance(const vec3 c)
{
    return sqrt( 0.299*c.r*c.r + 0.587*c.g*c.g + 0.114*c.b*c.b);
}

vec3 rotate(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

vec2 findUV(vec4 quat, vec3 N)
{
    vec3 gN = rotate(quat, N);
    float theta = atan(gN.y, gN.x);
    float phi = asin(gN.z);
    return vec2(fract(theta / (2 * PI) + 0.5), phi / PI + 0.5);
}

void makeOrthBase(in vec3 N, out vec3 T, out vec3 B)
{
    T = normalize(cross(N, abs(N.z) < 0.9 ? vec3(0, 0, 1) : vec3(1, 0, 0)));
    B = normalize(cross(N, T));
}

vec2 sampleBlueNoise(uint frame, uint depth, bool isShadow)
{
    uint cycle = frame / 64;

    uint haltonIndex = cycle % 64;
    vec2 haltonSample = halton[haltonIndex];
    ivec2 haltonOffset = ivec2(haltonSample * 64);
    ivec2 blueNoiseXY = (ivec2(gl_GlobalInvocationID.xy) + haltonOffset) % 64;

    uint bluneNoiseIndex = (frame + depth) % 64;
    vec4 blueNoiseChannels = imageLoad(blueNoise[bluneNoiseIndex], blueNoiseXY);
    vec2 blueNoise = (!isShadow ? blueNoiseChannels.rg : blueNoiseChannels.ba);

    vec2 goldenScramble = GOLDEN_RATIO * vec2(1, 2) * cycle;
    vec2 noise = fract(blueNoise + goldenScramble);

    return noise;
}

float balanceHeuristic(int nf, float fPdf, int ng, float gPdf)
{
    return (nf * fPdf) / (nf * fPdf + ng * gPdf);
}

float powerHeuristic(int nf, float fPdf, int ng, float gPdf)
{
    float f = nf * fPdf, g = ng * gPdf;
    return (f * f) / (f * f + g * g);
}

float misHeuristic(int nf, float fPdf, int ng, float gPdf)
{
    return powerHeuristic(nf, fPdf, ng, gPdf);
}

float uniformConePdf(float cosThetaMax)
{
    return 1 / (2 * PI * (1 - cosThetaMax));
}

float uniformSpherePdf(vec3 center, float radius, vec3 point)
{
    float sinThetaMax2 = radius * radius / distanceSqr(point, center);
    float cosThetaMax = sqrt(max(0, 1 - sinThetaMax2));
    return uniformConePdf(cosThetaMax);
}

// Also used for Lambert BRDF
float cosineHemispherePdf(float cosTheta)
{
    return cosTheta / PI;
}

vec3 fresnel(float HdotV, vec3 F0)
{
    float s = 1 - HdotV;
    float s2 = s * s;
    float s5 = s2 * s2 * s;
    return F0 + (1 - F0) * s5;
}

float ggxNDF(float a2, float NdotH)
{
    return a2 / (PI * sqr(NdotH*NdotH * (a2-1) + 1));
}

float ggxSmith(float a2, float NoV, float NoL)
{
    float G_V = NoV + sqrt( (NoV - NoV * a2) * NoV + a2 );
    float G_L = NoL + sqrt( (NoL - NoL * a2) * NoL + a2 );
    return 1.0 / ( G_V * G_L );
}

float ggxBsdf(float a, float NoV, float NoL, float NdotH)
{
    float a2 = a*a;
    return ggxNDF(a2, NdotH) * ggxSmith(a2, NoV, NoL);
}

vec3 sampleGGXVNDF(vec3 V_, float alpha_x, float alpha_y, float U1, float U2)
{
    // stretch view
    vec3 V = normalize(vec3(alpha_x * V_.x, alpha_y * V_.y, V_.z));
    // orthonormal basis
    vec3 T1 = (V.z < 0.9999) ? normalize(cross(V, vec3(0,0,1))) : vec3(1,0,0);
    vec3 T2 = cross(T1, V);
    // sample point with polar coordinates (r, phi)
    float a = 1.0 / (1.0 + V.z);
    float r = sqrt(U1);
    float phi = (U2<a) ? U2/a * PI : PI + (U2-a)/(1.0-a) * PI;
    float P1 = r*cos(phi);
    float P2 = r*sin(phi)*((U2<a) ? 1.0 : V.z);
    // compute normal
    vec3 N = P1*T1 + P2*T2 + sqrt(max(0.0, 1.0 - P1*P1 - P2*P2))*V;
    // unstretch
    N = normalize(vec3(alpha_x*N.x, alpha_y*N.y, max(0.0, N.z)));
    return N;
}

Ray genRay(uvec2 pixelPos)
{
    vec2 noiseOffset = sampleBlueNoise(frameIndex, 0, false);

    vec4 pixelClip = vec4(
        float(pixelPos.x) + noiseOffset.x,
        float(pixelPos.y) + noiseOffset.y,
        0, 1);

    vec4 rayDir = rayMatrix * pixelClip;

    Ray ray;
    ray.direction = normalize(rayDir.xyz / rayDir.w);
    ray.origin = eyePosition.xyz;
    ray.throughput = vec3(1, 1, 1);
    ray.depth = 0;
    ray.bsdfPdf = DELTA;

    return ray;
}

Intersection intersects(in Ray ray, in uint instanceId)
{
    Intersection intersection;
    intersection.t = -1.0;
    intersection.instanceId = instanceId;

    Instance instance = instances[instanceId];

    vec3 O = ray.origin;
    vec3 D = ray.direction;
    vec3 C = instance.position.xyz;

    vec3 L = C - O;
    float t_ca = dot(L, D);

    if(t_ca < 0)
        return intersection;

    float dSqr = (dot(L, L) - t_ca*t_ca);

    if(dSqr > instance.radius * instance.radius * (radiusScale * radiusScale))
        return intersection;

    float t_hc = sqrt(instance.radius * instance.radius * (radiusScale * radiusScale) - dSqr);
    float t_0 = t_ca - t_hc;
    float t_1 = t_ca + t_hc;

    if(t_0 > 0)
        intersection.t = t_0;
    else if(t_1 > 0)
        intersection.t = t_1;

    return intersection;
}

Intersection raycast(in Ray ray)
{
    Intersection bestIntersection;
    bestIntersection.t = 1 / 0.0;

    for(uint b = 0; b < instanceCount; ++b)
    {
        Intersection intersection = intersects(ray, b);

        if(intersection.t > 0 && intersection.t < bestIntersection.t)
        {
            bestIntersection = intersection;
        }
    }

    return bestIntersection;
}

HitInfo probe(in Ray ray, in Intersection intersection)
{
    Instance instance = instances[intersection.instanceId];

    HitInfo hitInfo;

    hitInfo.instanceId = intersection.instanceId;

    hitInfo.position = ray.origin + intersection.t * ray.direction;
    hitInfo.normal = normalize(hitInfo.position - instance.position.xyz);

    hitInfo.roughness = instance.specular.r;
    hitInfo.metalness = instance.specular.g;
    hitInfo.reflectance = instance.specular.b;

    vec3 albedo = instance.albedo.rgb;
    if(instance.materialId > 0)
    {
        vec2 uv = findUV(instance.quaternion, hitInfo.normal);
        layout(rgba8) image2D albedoImg = materials[instance.materialId-1].albedo;
        ivec2 imgSize = imageSize(albedoImg);
        vec3 texel = imageLoad(albedoImg, ivec2(imgSize * vec2(uv.x, 1 - uv.y))).rgb;

        albedo = toLinear(texel);
    }

    hitInfo.diffuseAlbedo = mix(albedo, vec3(0, 0, 0), hitInfo.metalness);
    hitInfo.specularF0 = mix(hitInfo.reflectance.xxx, albedo, hitInfo.metalness);
    hitInfo.emission = instance.emission.rgb;

    float iRSqr = instance.radius * instance.radius;
    vec3 originToHit = (instance.position.xyz - ray.origin);
    float distanceSqr = dot(originToHit, originToHit);
    float sinThetaMaxSqr = iRSqr / distanceSqr;
    float cosThetaMax = sqrt(max(0, 1 - sinThetaMaxSqr));
    float solidAngle = 2 * (1 - cosThetaMax);
    hitInfo.areaPdf = solidAngle;

    return hitInfo;
}

vec3 shade(in Ray ray, in HitInfo hitInfo)
{
    float NdotV = max(0, -dot(hitInfo.normal, ray.direction));

    vec3 L_out = vec3(0, 0, 0);

    for(uint e = emitterStart; e < emitterEnd; ++e)
    {
        if(e == hitInfo.instanceId)
            continue;

        Instance emitter = instances[e];

        float lRSqr = emitter.radius * emitter.radius;

        vec3 hitToLight = (emitter.position.xyz - hitInfo.position);
        float distanceSqr = dot(hitToLight, hitToLight);

        float sinThetaMaxSqr = lRSqr / distanceSqr;
        float cosThetaMax = sqrt(max(0, 1 - sinThetaMaxSqr));

        float solidAngle = float(2 * (1 - cosThetaMax));

        // Importance sample cone
        vec2 noise = sampleBlueNoise(frameIndex, ray.depth, true);

        float cosTheta = (1 - noise.x) + noise.x * cosThetaMax;
        float sinTheta = sqrt(max(0, 1 - cosTheta*cosTheta));
        float phi = noise.y * 2 * PI;

        float dc = sqrt(distanceSqr);
        float ds = dc * cosTheta - sqrt(max(0, lRSqr - dc * dc * sinTheta * sinTheta));
        float cosAlpha = (dc * dc + lRSqr - ds * ds) / (2 * dc * emitter.radius);
        float sinAlpha = sqrt(max(0, 1 - cosAlpha * cosAlpha));

        vec3 lT, lB, lN = -normalize(hitToLight);
        makeOrthBase(lN, lT, lB);

        vec3 lightN = emitter.radius * vec3(sinAlpha * cos(phi), sinAlpha * sin(phi), cosAlpha);
        vec3 lightP = lightN.x * lT + lightN.y * lB + lightN.z * lN;
        vec3 L = normalize(hitToLight + lightP);

        vec3 H = normalize(L - ray.direction);
        float NdotL = max(0, dot(hitInfo.normal, L));
        float NdotH = dot(hitInfo.normal, H);
        float HdotV = -dot(H, ray.direction);

        vec3 f = fresnel(HdotV, hitInfo.specularF0);

        float diffuseWeight = misHeuristic(1, 1/solidAngle, 1, cosineHemispherePdf(NdotL));
        vec3 diffuse = hitInfo.diffuseAlbedo * (1 - f) * diffuseWeight;

        vec3 specularAlbedo = f * ggxBsdf(hitInfo.roughness, NdotV, NdotL, NdotH);
        float specularWeight = 1;
        vec3 specular = (NdotL != 0 && NdotV != 0) ? specularAlbedo * specularWeight : vec3(0, 0, 0);

        Ray shadowRay;
        shadowRay.origin = hitInfo.position;
        shadowRay.direction = L;

        Intersection shadowIntersection = raycast(shadowRay);
        if(shadowIntersection.instanceId == e || shadowIntersection.t == 1 / 0.0)
        {
            // Simplified PI in solid angle and lambert brdf
            L_out += NdotL * (diffuse + specular) * emitter.emission.rgb * solidAngle;
        }
    }

    vec3 shading = ray.throughput * L_out;

    // Emission
    float lambertWeight = ray.bsdfPdf != DELTA ? misHeuristic(1, ray.bsdfPdf, 1, 1/hitInfo.areaPdf) : 1;
    shading += lambertWeight * hitInfo.emission;

    return shading;
}

vec3 shadeBackground(in Ray ray)
{
    vec2 uv = findUV(backgroundQuat, ray.direction);
    ivec2 imgSize = imageSize(backgroundImg);
    vec3 lum = imageLoad(backgroundImg, ivec2(imgSize * vec2(uv.x, 1 - uv.y))).rgb;

    return toLinear(lum) * ray.throughput;
}

Ray reflect(Ray rayIn, HitInfo hitInfo)
{
    Ray rayOut = rayIn;
    rayOut.origin = hitInfo.position + hitInfo.normal * 1e-9;
    rayOut.depth = rayIn.depth + 1;

    vec2 noise = sampleBlueNoise(frameIndex, rayOut.depth, false);

    vec3 T, B, N = hitInfo.normal;
    makeOrthBase(N, T, B);

    // Specular
    vec3 V = -vec3(dot(rayIn.direction, T), dot(rayIn.direction, B), dot(rayIn.direction, N));
    vec3 H = sampleGGXVNDF(V, hitInfo.roughness, hitInfo.roughness, noise.x, noise.y);
    vec3 L = 2.0 * dot(H, V) * H - V;
    vec3 reflectionDirection = normalize(L.x * T + L.y * B + L.z * N);
    vec3 specularAlbedo = fresnel(max(0, dot(H, V)), hitInfo.specularF0);
    float f = L.z > 0 ? toLuminance(specularAlbedo) : 0;

    vec2 specularNoise = sampleBlueNoise(frameIndex + 32, rayOut.depth, false);

    if(specularNoise.x < f)
    {
        float NdotL = max(0, L.z);
        float NdotV = max(0, V.z);
        float NdotH = max(0, H.z);
        float HdotV = max(0, dot(H, V));

        vec3 ggxAlbedo = specularAlbedo;

        rayOut.direction = reflectionDirection;
        rayOut.bsdfPdf = 0;
        rayOut.throughput = rayIn.throughput * ggxAlbedo / f;
    }
    else if(specularNoise.x >= f && f != 1)
    {
        // Diffuse
        float angle = noise.r * 2 * PI;
        float radius = sqrt(noise.g);
        vec2 xy = vec2(cos(angle), sin(angle)) * radius;
        float z = sqrt(max(0, 1 - radius * radius));
        rayOut.direction = normalize(xy.x * T + xy.y * B + z * N);
        rayOut.bsdfPdf = cosineHemispherePdf(z);
        rayOut.throughput = rayIn.throughput * hitInfo.diffuseAlbedo / (1 - f);
    }
    else
    {
        rayOut.bsdfPdf = 0;
        rayOut.throughput = vec3(0, 0, 0);
    }

    return rayOut;
}

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

void main()
{
    vec3 colorAccum = vec3(0, 0, 0);

    Ray ray = genRay(gl_GlobalInvocationID.xy);

    for(uint depthId = 0; depthId < PATH_LENGTH; ++depthId)
    {
        Intersection bestIntersection = raycast(ray);

        if (bestIntersection.t != 1 / 0.0)
        {
            HitInfo hitInfo = probe(ray, bestIntersection);
            colorAccum += shade(ray, hitInfo);
            ray = reflect(ray, hitInfo);
        }
        else
        {
            colorAccum += shadeBackground(ray);
            break;
        }
    }

    vec3 finalLinear = exposure * colorAccum;

    if(frameIndex != 0)
    {
        vec4 prevFrameSRGB = imageLoad(result, ivec2(gl_GlobalInvocationID));
        vec3 prevFrameLinear = toLinear(prevFrameSRGB.rgb);

        float blend = frameIndex / float(frameIndex+1);
        finalLinear = mix(finalLinear, prevFrameLinear, blend);
    }

    vec4 finalSRGB = vec4(sRGB(finalLinear), 0);
    imageStore(result, ivec2(gl_GlobalInvocationID), finalSRGB);
}
