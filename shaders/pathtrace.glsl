#version 440
#extension GL_ARB_bindless_texture : require

#define PI 3.14159265359
#define GOLDEN_RATIO 1.618033988749894
#define DELTA (1 / 0.0)

#define IS_UNBIASED 0
const uint PATH_LENGTH = 4;

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
#if !IS_UNBIASED
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

layout (std140, binding = 0) uniform CommonParams
{
    mat4 rayMatrix;
    vec4 eyePosition;
    float exposure;
    uint frameIndex;

    int pad1;

    // Background
    float backgroundExposure;
    vec4 backgroundQuat;

    layout(rgba8) readonly image2D blueNoise[64];

    vec2 halton[64];
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

uniform layout(binding = 0) sampler2D backgroundImg;

uniform layout(binding = 6, rgba32f) image2D result;

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

// MIS heuristics
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


// Also used for Lambert BRDF
float cosineHemispherePdf(float cosTheta)
{
    return cosTheta / PI;
}

// Specular BSDF
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

float ggxSmithG1(float a2, float NoV)
{
    return  2 * NoV / (NoV + sqrt((NoV - NoV * a2) * NoV + a2));
}

float ggxSmithG2(float a2, float NoV, float NoL)
{
    float G_V = NoV + sqrt( (NoV - NoV * a2) * NoV + a2 );
    float G_L = NoL + sqrt( (NoL - NoL * a2) * NoL + a2 );
    return  4 * NoV * NoL / ( G_V * G_L );
}

float ggxSmithG2AndDenom(float a2, float NoV, float NoL)
{
    float G_V = NoV + sqrt( (NoV - NoV * a2) * NoV + a2 );
    float G_L = NoL + sqrt( (NoL - NoL * a2) * NoL + a2 );
    return 1 / ( G_V * G_L );
}

float ggxVisibleNormalsDistributionFunction(float a2, float NoV, float NdotH, float HdotV)
{
    return ggxSmithG1(a2, NoV) * max(0, HdotV) * ggxNDF(a2, NdotH) / NoV;
}

vec3 sampleGGXVNDF(vec3 V_, float alpha_x, float alpha_y, float U1, float U2)
{
    // stretch view
    vec3 T1, T2, V = normalize(vec3(alpha_x * V_.x, alpha_y * V_.y, V_.z));
    makeOrthBase(V, T1, T2);

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

vec3 sampleCosineHemisphere(float U1, float U2)
{
    float angle = U1 * 2 * PI;
    float radius = sqrt(U2);
    vec2 xy = vec2(cos(angle), sin(angle)) * radius;
    float z = sqrt(max(0, 1 - radius * radius));
    return vec3(xy, z);
}

vec3 sampleUniformCone(float U1, float U2, float cosThetaMax)
{
    float phi = U2 * 2 * PI;
    float cosTheta = (1 - U1) + U1 * cosThetaMax;
    float sinTheta = sqrt(1 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
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
#if !IS_UNBIASED
    ray.diffusivity = 0;
#endif
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

    if(dSqr > instance.radius * instance.radius)
        return intersection;

    float t_hc = sqrt(instance.radius * instance.radius - dSqr);
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

    for(uint b = 0; b < instances.length(); ++b)
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

    vec3 albedo = instance.albedo.rgb;
    if(instance.materialId > 0)
    {
        vec2 uv = findUV(instance.quaternion, hitInfo.normal);
        layout(rgba8) image2D albedoImg = materials[instance.materialId-1].albedo;
        ivec2 imgSize = imageSize(albedoImg);
        vec3 texel = imageLoad(albedoImg, ivec2(imgSize * vec2(uv.x, 1 - uv.y))).rgb;

        albedo = toLinear(texel);
    }

    hitInfo.specularA = instance.specular.r;
    hitInfo.specularA2 = hitInfo.specularA * hitInfo.specularA;

    float metalness = instance.specular.g;
    float reflectance = instance.specular.b;

    hitInfo.diffuseAlbedo = mix(albedo, vec3(0, 0, 0), metalness);
    hitInfo.specularF0 = mix(reflectance.xxx, albedo, metalness);
    hitInfo.emission = instance.emission.rgb;

    hitInfo.NdotV = max(0, -dot(hitInfo.normal, ray.direction));

    float iRSqr = instance.radius * instance.radius;
    vec3 originToHit = (instance.position.xyz - ray.origin);
    float distanceSqr = dot(originToHit, originToHit);
    float sinThetaMaxSqr = iRSqr / distanceSqr;
    float cosThetaMax = sqrt(max(0, 1 - sinThetaMaxSqr));
    float solidAngle = 2 * PI * (1 - cosThetaMax);
    hitInfo.instanceAreaPdf = 1 / solidAngle;

    return hitInfo;
}

vec3 evaluateBSDF(Ray ray, HitInfo hitInfo, vec3 L, float solidAngle)
{
    float lightAreaPdf = 1 / solidAngle;

    vec3 H = normalize(L - ray.direction);
    float NdotL = max(0, dot(hitInfo.normal, L));
    float NdotH = dot(hitInfo.normal, H);
    float HdotV = -dot(H, ray.direction);
    float NdotV = hitInfo.NdotV;

    vec3 f = fresnel(HdotV, hitInfo.specularF0);
    float ndf = ggxNDF(hitInfo.specularA2, NdotH);

    float diffuseWeight = misHeuristic(1, lightAreaPdf, 1, cosineHemispherePdf(NdotL));
    vec3 diffuse = hitInfo.diffuseAlbedo / PI * (1 - f) * diffuseWeight;

    vec3 specularAlbedo = f * ndf * ggxSmithG2AndDenom(hitInfo.specularA2, NdotV, NdotL);
    float specularPdf = ggxVisibleNormalsDistributionFunction(hitInfo.specularA2, NdotV, NdotH, HdotV) / (4 * HdotV);
    float specularWeight = hitInfo.specularA != 0 ? misHeuristic(1, lightAreaPdf, 1, specularPdf) : 0;
    vec3 specular = (NdotL > 0 && NdotV > 0 && hitInfo.specularA2 != 0) ? specularAlbedo * specularWeight : vec3(0, 0, 0);

    return NdotL * (diffuse + specular) * solidAngle;
}

vec3 sampleSphereLight(uint emitterId, Ray ray, HitInfo hitInfo)
{
    Instance emitter = instances[emitterId];

    float lRSqr = emitter.radius * emitter.radius;

    vec3 hitToLight = (emitter.position.xyz - hitInfo.position);
    float distanceSqr = dot(hitToLight, hitToLight);

    float sinThetaMaxSqr = lRSqr / distanceSqr;
    float cosThetaMax = sqrt(max(0, 1 - sinThetaMaxSqr));

    float solidAngle = 2 * PI * (1 - cosThetaMax);

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

    Ray shadowRay;
    shadowRay.origin = hitInfo.position;
    shadowRay.direction = L;

    Intersection shadowIntersection = raycast(shadowRay);
    if(shadowIntersection.instanceId == emitterId)
        return evaluateBSDF(ray, hitInfo, L, solidAngle) * emitter.emission.rgb;
    else
        return vec3(0);
}

vec3 sampleDirectionalLight(uint lightId, Ray ray, HitInfo hitInfo)
{
    DirectionalLight light = directionalLights[lightId];

    vec3 T, B, N = light.directionCosThetaMax.xyz;
    makeOrthBase(N, T, B);

    vec2 noise = sampleBlueNoise(frameIndex, ray.depth, true);
    vec3 l = sampleUniformCone(noise.r, noise.g, light.directionCosThetaMax.w);
    vec3 L = normalize(l.x * T + l.y * B + l.z * N);

    Ray shadowRay;
    shadowRay.origin = hitInfo.position;
    shadowRay.direction = L;

    Intersection shadowIntersection = raycast(shadowRay);
    if(shadowIntersection.t == 1 / 0.0)
        return evaluateBSDF(ray, hitInfo, L, light.emissionSolidAngle.a) * light.emissionSolidAngle.rgb;
    else
        return vec3(0);
}

vec3 shadeHit(Ray ray, HitInfo hitInfo)
{
    vec3 L_out = vec3(0);

    for(uint e = 0; e < emitters.length(); ++e)
    {
        uint emitterId = emitters[e];

        if(emitterId == hitInfo.instanceId)
            continue;

        L_out += sampleSphereLight(emitterId, ray, hitInfo);
    }

    for(uint dl = 0; dl < directionalLights.length(); ++dl)
    {
        L_out += sampleDirectionalLight(dl, ray, hitInfo);
    }

    // Emission
    float weight = ray.bsdfPdf != DELTA ? misHeuristic(1, ray.bsdfPdf, 1, hitInfo.instanceAreaPdf) : 1;
    L_out += weight * hitInfo.emission;

    return ray.throughput * L_out;
}

vec3 shadeSky(in Ray ray)
{
    vec2 uv = findUV(backgroundQuat, ray.direction);
    vec3 skyLuminance = texture2D(backgroundImg, vec2(uv.x, 1 - uv.y)).rgb;

    vec3 L_in = toLinear(skyLuminance) * backgroundExposure;

    for(uint dl = 0; dl < directionalLights.length(); ++dl)
    {
        DirectionalLight light = directionalLights[dl];
        float cosTheta = dot(light.directionCosThetaMax.xyz, ray.direction);

        if(cosTheta >= light.directionCosThetaMax.w)
        {
            float lightAreaPdf = 1 / light.emissionSolidAngle.a;
            float weight = ray.bsdfPdf != DELTA ? misHeuristic(1, ray.bsdfPdf, 1, lightAreaPdf) : 1;
            L_in += weight * light.emissionSolidAngle.rgb;
        }
    }

    return ray.throughput * L_in;
}

Ray scatter(Ray rayIn, HitInfo hitInfo)
{
    Ray rayOut = rayIn;
    rayOut.origin = hitInfo.position + hitInfo.normal * 1e-9;
    rayOut.depth = rayIn.depth + 1;

    vec2 noise = sampleBlueNoise(frameIndex, rayOut.depth, false);

    vec3 T, B, N = hitInfo.normal;
    makeOrthBase(N, T, B);

    // Specular
    vec3 V = -vec3(dot(rayIn.direction, T), dot(rayIn.direction, B), dot(rayIn.direction, N));
    vec3 H = sampleGGXVNDF(V, hitInfo.specularA, hitInfo.specularA, noise.x, noise.y);
    vec3 L = 2.0 * dot(H, V) * H - V;
    vec3 reflectionDirection = normalize(L.x * T + L.y * B + L.z * N);

    vec3 f = fresnel(max(0, dot(H, V)), hitInfo.specularF0);
    vec3 specularAlbedo = f
                * ggxSmithG2(hitInfo.specularA2, V.z, L.z)
                / ggxSmithG1(hitInfo.specularA2, V.z);
    float specularLuminance = L.z > 0 ? toLuminance(specularAlbedo) : 0;
    float diffuseLuminance = toLuminance(hitInfo.diffuseAlbedo);

    float r = specularLuminance != 0 ? specularLuminance / (specularLuminance + diffuseLuminance) : 0;

    vec2 specularNoise = sampleBlueNoise(frameIndex + 32, rayOut.depth, false);

    if(specularNoise.x < r)
    {
        float HdotV = dot(V, H);
        float pdf = ggxVisibleNormalsDistributionFunction(hitInfo.specularA2, V.z, H.z, HdotV) / (4 * HdotV);

        rayOut.direction = reflectionDirection;
        rayOut.bsdfPdf = hitInfo.specularA == 0 ? DELTA : pdf;
        rayOut.throughput = rayIn.throughput * specularAlbedo / r;
#if !IS_UNBIASED
        rayOut.throughput *= mix(1 - rayIn.diffusivity, 1, min(1, hitInfo.specularA));
        rayOut.diffusivity = mix(rayIn.diffusivity, 1, min(1, hitInfo.specularA));
#endif
    }
    else if(specularNoise.x >= r && r != 1)
    {
        // Diffuse
        vec3 dir = sampleCosineHemisphere(noise.r, noise.g);
        rayOut.direction = normalize(dir.x * T + dir.y * B + dir.z * N);
        rayOut.bsdfPdf = cosineHemispherePdf(dir.z);
        rayOut.throughput = rayIn.throughput * hitInfo.diffuseAlbedo * (1 - f) / (1 - r);
#if !IS_UNBIASED
        rayOut.diffusivity = 1;
#endif
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
            colorAccum += shadeHit(ray, hitInfo);
            ray = scatter(ray, hitInfo);
        }
        else
        {
            colorAccum += shadeSky(ray);
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
