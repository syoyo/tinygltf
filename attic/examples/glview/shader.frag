uniform sampler2D diffuseTex;
uniform int uIsCurve;

varying vec3 normal;
varying vec2 texcoord;

void main(void)
{
    //gl_FragColor = vec4(0.5 * normalize(normal) + 0.5, 1.0);
    //gl_FragColor = vec4(texcoord, 0.0, 1.0);
    if (uIsCurve > 0) {
        gl_FragColor = texture2D(diffuseTex, texcoord);
    } else {
        gl_FragColor = vec4(0.5 * normalize(normal) + 0.5, 1.0);
    }
}
