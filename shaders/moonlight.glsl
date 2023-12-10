uniform sampler2D albedo;

uniform layout(binding = 0, rgba32f) image2D lighting;

layout (std140, binding = 0) uniform CommonParams
{
    mat4 transform;
    vec4 sunDirection;
    vec4 moonDirection;
    vec4 sunLi;
};

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
    vec2 uv = vec2(gl_GlobalInvocationID.xy) / vec2(gl_NumWorkGroups.xy * uvec2(8, 8));

    vec2 clipPos = uv * 2 - 1;
    vec3 dirMoon = normalize(vec3(clipPos.x, clipPos.y, sqrt(1 - dot(clipPos, clipPos))));
    vec3 dirEarth = (transform * vec4(dirMoon, 0)).xyz;

    vec3 moonAlbedo = texture(albedo, vec2(uv.x, 1 - uv.y)).rgb;

    float backScattering = 2.0f + dot(-moonDirection.xyz, sunDirection.xyz) / 3.0f;
    vec3 L_o = moonAlbedo * sunLi.rgb * max(0, dot(dirEarth, sunDirection.xyz)) * backScattering;

    imageStore(lighting, ivec2(gl_GlobalInvocationID), vec4(L_o, 0));
}
