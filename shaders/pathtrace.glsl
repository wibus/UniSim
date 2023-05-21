#version 440
#extension GL_ARB_bindless_texture : require

#define PI 3.14159265359

struct Instance
{
    vec4 albedo;
    vec4 emission;
    vec4 position;
    vec4 quaternion;

    float radius;
    float mass;

    uint materialId;

    float pad1;
};

struct Material
{
    layout(rgba8) image2D albedo;
    layout(rgba8) image2D specular;
};

struct Ray
{
    vec3 origin;
    vec3 direction;
    vec3 albedo;
    uint depth;
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
    vec3 albedo;
    vec3 emission;
    float metalness;
    float roughness;
};

layout (std140, binding = 0) uniform CommonParams
{
    mat4 invViewMat;
    mat4 rayMatrix;
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
    layout(rgba8) image2D backgroundImg;

    layout(rgba8) image2D blueNoise[64];

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

uniform layout(binding = 3, rgba16f) image2D result;

const uint PATH_DEPTH = 4;

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

vec3 rotate(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

vec2 findUV(vec4 quat, vec3 N)
{
    vec3 gN = rotate(quat, mat3(invViewMat) * N);
    float theta = atan(gN.y, gN.x);
    float phi = asin(gN.z);
    return vec2(fract(theta / (2 * PI) + 0.5), phi / PI + 0.5);
}

vec2 sampleBlueNoise(uint frame, uint depth, bool isShadow)
{
    uint haltonIndex = min(63, frame / 64);
    vec2 haltonSample = halton[haltonIndex];
    ivec2 haltonOffset = ivec2(haltonSample * 64);
    ivec2 blueNoiseXY = (ivec2(gl_GlobalInvocationID.xy) + haltonOffset) % 64;

    uint bluneNoiseIndex = (frame + depth) % 64;
    vec4 noise = imageLoad(blueNoise[bluneNoiseIndex], blueNoiseXY);
    return !isShadow ? noise.rg : noise.ba;
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
    ray.origin = vec3(0, 0, 0);
    ray.albedo = vec3(1, 1, 1);
    ray.depth = 0;

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

    hitInfo.albedo = instance.albedo.rgb;

    if(instance.materialId > 0)
    {
        vec2 uv = findUV(instance.quaternion, hitInfo.normal);
        layout(rgba8) image2D albedoImg = materials[instance.materialId-1].albedo;
        ivec2 imgSize = imageSize(albedoImg);
        vec3 texel = imageLoad(albedoImg, ivec2(imgSize * vec2(uv.x, 1 - uv.y))).rgb;

        hitInfo.albedo = toLinear(texel);
    }

    hitInfo.emission = instance.emission.rgb;

    hitInfo.metalness = 0;
    hitInfo.roughness = 1;

    return hitInfo;
}

vec3 shade(in Ray ray, in HitInfo hitInfo)
{
    vec3 N = hitInfo.normal;
    vec3 L_out = vec3(0, 0, 0);

    for(uint e = emitterStart; e < emitterEnd; ++e)
    {
        if(e == hitInfo.instanceId)
            continue;

        Instance emitter = instances[e];

        Ray shadowRay;
        shadowRay.origin = hitInfo.position + hitInfo.normal * 1e-10;

        vec2 noise = sampleBlueNoise(frameIndex, ray.depth, true);
        float theta = (noise.x - 0.5) * PI;
        float cosTheta = cos(theta);
        float phi = noise.y * 2 * PI;
        vec3 samplePos = vec3(cosTheta * cos(phi), cosTheta * sin(phi), sin(cosTheta)) * emitter.radius;
        shadowRay.direction = normalize(emitter.position.xyz + samplePos - hitInfo.position);

        if(raycast(shadowRay).instanceId != e)
            continue;

        vec3 dist = (emitter.position.xyz - hitInfo.position);
        float distanceSqr = dot(dist, dist);
        float distance = sqrt(distanceSqr);

        vec3 I = dist / distance;
        float LdotN = dot(I, N);

        if(LdotN <= 0)
            continue;

        float radiusRatio = emitter.radius / distance;
        float solidAngle = float(2 * (1 - sqrt(1 - double(radiusRatio*radiusRatio))));

        // Simplified PI in solid angle and lambert brdf
        L_out += LdotN * emitter.emission.rgb * solidAngle;
    }

    vec3 albedo = hitInfo.albedo * ray.albedo;
    ray.albedo = albedo;

    vec3 shading = L_out * albedo;

    if(ray.depth == 0)
        shading += hitInfo.emission;

    return shading;
}

vec3 shadeBackground(in Ray ray)
{
    vec2 uv = findUV(backgroundQuat, ray.direction);
    ivec2 imgSize = imageSize(backgroundImg);
    vec3 lum = imageLoad(backgroundImg, ivec2(imgSize * vec2(uv.x, 1 - uv.y))).rgb;

    return toLinear(lum) * ray.albedo;
}

Ray reflect(Ray rayIn, HitInfo hitInfo)
{
    Ray rayOut = rayIn;

    rayOut.origin = hitInfo.position + hitInfo.normal * 1e-9;
    rayOut.albedo = rayIn.albedo * hitInfo.albedo;
    rayOut.depth = rayIn.depth + 1;

    vec2 noise = sampleBlueNoise(frameIndex, rayOut.depth, false);

    float angle = noise.r * 2 * PI;
    float radius = sqrt(noise.g);
    vec2 xy = vec2(cos(angle), sin(angle)) * radius;
    float z = sqrt(max(0, 1 - radius * radius));

    vec3 normal = hitInfo.normal;
    vec3 tangent = normalize(cross(normal, abs(normal.z) < 0.9 ? vec3(0, 0, 1) : vec3(1, 0, 0)));
    vec3 binormal = normalize(cross(normal, tangent));

    rayOut.direction = normalize(xy.x * tangent + xy.y * binormal + z * normal);

    return rayOut;
}

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;

void main()
{
    vec3 colorAccum = vec3(0, 0, 0);

    Ray ray = genRay(gl_GlobalInvocationID.xy);

    for(uint depthId = 0; depthId < PATH_DEPTH; ++depthId)
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
