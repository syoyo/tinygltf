attribute vec3    in_vertex;
attribute vec3    in_normal;
attribute vec2    in_texcoord;

varying vec3      normal;
varying vec2      texcoord;

void main(void)
{
	vec4 p = gl_ModelViewProjectionMatrix * vec4(in_vertex, 1);
	gl_Position = p;
	vec4 nn = gl_ModelViewMatrixInverseTranspose * vec4(normalize(in_normal), 0);
	normal = nn.xyz;

	texcoord = in_texcoord;
}
