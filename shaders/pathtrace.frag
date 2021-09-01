#version 440
#extension GL_ARB_bindless_texture : require

#define PI 3.14159265359

struct Body
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
};

struct Intersection
{
    float t;
    uint bodyId;
};

layout (std140, binding = 0) uniform CommonParams
{
    mat4 invViewMat;
    mat4 rayMatrix;
    uint bodyCount;
    float radiusScale;
    float exposure;
    uint emitterStart;
    uint emitterEnd;

    uint pad0;
    uint pad1;
    uint pad2;

    // Background
    vec4 backgroundQuat;
    layout(rgba8) image2D backgroundImg;
};

layout (std140, binding = 1) uniform Bodies
{
    Body bodies[100];
};

layout (std140, binding = 2) uniform Materials
{
    Material materials[100];
};

out vec4 frag_colour;

const uint SAMPLE_WIDTH = 4;
const uint SAMPLE_COUNT = SAMPLE_WIDTH * SAMPLE_WIDTH;

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

Ray genRay(uint rayId)
{
    vec4 pixelPos = vec4(
        gl_FragCoord.x + (float(rayId % SAMPLE_WIDTH) + 0.5) / float(SAMPLE_WIDTH) - 0.5,
        gl_FragCoord.y + (float(rayId / SAMPLE_WIDTH) + 0.5) / float(SAMPLE_WIDTH) - 0.5,
        0, 1);

    vec4 rayDir = rayMatrix * pixelPos;

    Ray ray;
    ray.direction = normalize(rayDir.xyz / rayDir.w);
    ray.origin = vec3(0, 0, 0);

    return ray;
}

Intersection intersects(in Ray ray, in uint bodyId)
{
    Intersection intersection;
    intersection.t = -1.0;
    intersection.bodyId = bodyId;

    Body body = bodies[bodyId];

    vec3 O = ray.origin;
    vec3 D = ray.direction;
    vec3 C = body.position.xyz;

    vec3 L = C - O;
    float t_ca = dot(L, D);

    if(t_ca < 0)
        return intersection;

    float dSqr = (dot(L, L) - t_ca*t_ca);

    if(dSqr > body.radius * body.radius * (radiusScale * radiusScale))
        return intersection;

    float t_hc = sqrt(body.radius * body.radius * (radiusScale * radiusScale) - dSqr);
    float t_0 = t_ca - t_hc;
    float t_1 = t_ca + t_hc;

    if(t_0 > 0)
        intersection.t = t_0;
    else if(t_1 > 0)
        intersection.t = t_1;

    return intersection;
}

vec2 findUV(vec4 quat, vec3 N)
{
    vec3 gN = rotate(quat, mat3(invViewMat) * N);
    float theta = atan(gN.y, gN.x);
    float phi = asin(gN.z);
    return vec2(fract(theta / (2 * PI) + 0.5), phi / PI + 0.5);
}

vec3 shadeBody(in Ray ray, in Intersection intersection)
{
    Body body = bodies[intersection.bodyId];

    vec3 point = ray.origin + intersection.t * ray.direction;
    vec3 N = normalize(point - body.position.xyz);

    vec3 L_out = vec3(0, 0, 0);

    for(uint e = emitterStart; e < emitterEnd; ++e)
    {
        if(e == intersection.bodyId)
            continue;

        Body emitter = bodies[e];

        vec3 dist = (emitter.position.xyz - point);
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

    vec2 uv = findUV(body.quaternion, N);
    vec2 check = fract(vec2(uv.x * 24, uv.y * 12));

    vec3 albedo = body.albedo.rgb * check.x*check.y;
    if(body.materialId > 0)
    {
        layout(rgba8) image2D albedoImg = materials[body.materialId-1].albedo;
        ivec2 imgSize = imageSize(albedoImg);
        vec3 texel = imageLoad(albedoImg, ivec2(imgSize * vec2(uv.x, 1 - uv.y))).rgb;

        albedo = toLinear(texel);
    }

    return (L_out + body.emission.rgb) * albedo * exposure;
}

vec3 shadeBackground(in Ray ray)
{
    vec2 uv = findUV(backgroundQuat, ray.direction);
    ivec2 imgSize = imageSize(backgroundImg);
    vec3 lum = imageLoad(backgroundImg, ivec2(imgSize * vec2(uv.x, 1 - uv.y))).rgb;

    return toLinear(lum);
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0, 1);
}


void main()
{
    vec3 colorAccum = vec3(0, 0, 0);

    for(uint sampleId = 0; sampleId < SAMPLE_COUNT; ++sampleId)
    {
        Ray ray = genRay(sampleId);

        Intersection bestIntersection;
        bestIntersection.t = 1 / 0.0;

        for(uint b = 0; b < bodyCount; ++b)
        {
            Intersection intersection = intersects(ray, b);

            if(intersection.t > 0 && intersection.t < bestIntersection.t)
            {
                bestIntersection = intersection;
            }

        }

        if (bestIntersection.t != 1 / 0.0)
            colorAccum += shadeBody(ray, bestIntersection);
        else
            colorAccum += shadeBackground(ray);
    }

    vec3 finalLinear = colorAccum / float(SAMPLE_COUNT);
    vec3 finalAces = ACESFilm(finalLinear);
    vec3 finalSRGB = sRGB(finalAces);

    frag_colour = vec4(finalSRGB, 1.0);
}
