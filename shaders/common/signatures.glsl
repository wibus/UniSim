// Basics
float max3(float a, float b, float c);

float max4(float a, float b, float c, float d);

float maxV(vec2 v);

float maxV(vec3 v);

float maxV(vec4 v);

float sqr(float v);

float distanceSqr(vec3 a, vec3 b);

float sRGB(float x);

vec3 sRGB(vec3 c);

vec3 toLinear(const vec3 sRGB);

float toLuminance(const vec3 c);

vec3 rotate(vec4 q, vec3 v);

vec2 findUV(vec4 quat, vec3 N);

void makeOrthBase(in vec3 N, out vec3 T, out vec3 B);

vec4 sampleBlueNoise(uint depth);

// MIS heuristics
float balanceHeuristic(int nf, float fPdf, int ng, float gPdf);

float powerHeuristic(int nf, float fPdf, int ng, float gPdf);

float misHeuristic(int nf, float fPdf, int ng, float gPdf);


// Also used for Lambert BRDF
float cosineHemispherePdf(float cosTheta);

// Specular BSDF
vec3 fresnel(float HdotV, vec3 F0);

float ggxNDF(float a2, float NdotH);

float ggxSmithG1(float a2, float NoV);

float ggxSmithG2(float a2, float NoV, float NoL);

float ggxSmithG2AndDenom(float a2, float NoV, float NoL);

float ggxVisibleNormalsDistributionFunction(float a2, float NoV, float NdotH, float HdotV);

vec3 sampleGGXVNDF(vec3 V_, float alpha_x, float alpha_y, float U1, float U2);

vec3 sampleCosineHemisphere(float U1, float U2);

vec3 sampleUniformCone(float U1, float U2, float cosThetaMax);

vec2 sampleUniformDisk(float U1, float U2);


// SYSTEMS //

// Sky
void SampleSkyLuminance(
        out vec3 skyLuminance,
        out vec3 transmittance,
        vec3 cameraPos,
        vec3 viewDir);

void SampleSkyLuminanceToPoint(
        out vec3 skyLuminance,
        out vec3 transmittance,
        vec3 cameraPos,
        vec3 pointPos);

vec3 SampleDirectionalLightLuminance(
        vec3 viewDir,
        uint lightId);


// Terrain
Intersection intersectTerrain(in Ray ray, in uint instanceId);
