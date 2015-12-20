varying vec3 normal;

void main(void)
{
    gl_FragColor = vec4(0.5 * normalize(normal) + 0.5, 1.0);
}
