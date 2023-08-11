// Basics
float max3(float a, float b, float c)
{
    return max(max(a, b), c);
}

float max4(float a, float b, float c, float d)
{
    return max(max(a, b), max(c, d));
}

float maxV(vec2 v)
{
    return max(v.x, v.y);
}

float maxV(vec3 v)
{
    return max3(v.x, v.y, v.z);
}

float maxV(vec4 v)
{
    return max4(v.x, v.y, v.z, v.w);
}

void swap(inout float a, inout float b)
{
    float t = a;
    a = b;
    b = t;
}

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

vec4 quatConj(vec4 q)
{
    return vec4(-q.x, -q.y, -q.z, q.w);
}

vec3 rotate(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

vec2 findUV(vec3 N)
{
    float theta = atan(N.y, N.x);
    float phi = asin(N.z);
    return vec2(fract(theta / (2 * PI) + 0.5), phi / PI + 0.5);
}

vec2 findUV(vec4 quat, vec3 N)
{
    return findUV( rotate(quat, N) );
}

void makeOrthBase(in vec3 N, out vec3 T, out vec3 B)
{
    T = normalize(cross(N, abs(N.z) < 0.9 ? vec3(0, 0, 1) : vec3(1, 0, 0)));
    B = normalize(cross(N, T));
}

vec4 sampleBlueNoise(uint depth)
{
    uint cycle = frameIndex / 64;

    uint haltonIndex = cycle % 64;
    vec2 haltonSample = vec2(halton[haltonIndex]);
    ivec2 haltonOffset = ivec2(haltonSample * 64);
    ivec2 blueNoiseXY = (ivec2(gl_GlobalInvocationID.xy) + haltonOffset) % 64;

    uint bluenNoiseIndex = (frameIndex + depth) % 64;
    vec4 blueNoise = imageLoad(blueNoise[bluenNoiseIndex], blueNoiseXY);

    vec4 goldenScramble = GOLDEN_RATIO * vec4(1, 2, 3, 4) * cycle;
    vec4 noise = fract(blueNoise + goldenScramble);

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
    if(f != 0 && g != 0)
        return (f * f) / (f * f + g * g);
    else
        return 0;
}

float misHeuristic(int nf, float fPdf, int ng, float gPdf)
{
    if(fPdf == DELTA)
        return 1;
    else if(gPdf == DELTA)
        return 0;
    else
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

vec2 sampleUniformDisk(float U1, float U2)
{
    float r = sqrt(U1);
    float a = U2 * 2 * PI;
    return vec2(cos(a), sin(a)) * r;
}
