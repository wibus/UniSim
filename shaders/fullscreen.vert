layout (location = 0) in vec3 vp;

void main()
{
#ifdef API_OPENGL
    gl_Position = vec4(vec3(vp.x, -vp.y, vp.z * 2 - 1), 1.0);
#else
    gl_Position = vec4(vp, 1.0);
#endif
}
