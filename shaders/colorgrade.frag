#version 440


layout (binding = 0) uniform sampler2D input;

out vec4 frag_colour;


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
    vec3 inputSRGB = texelFetch(input, ivec2(gl_FragCoord), 0).rgb;
    vec3 inputLinear = toLinear(inputSRGB);
    vec3 finalAces = ACESFilm(inputLinear);
    vec3 finalSRGB = sRGB(finalAces);

    frag_colour = vec4(finalSRGB, 1.0);
}
