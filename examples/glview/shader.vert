attribute vec3    in_vertex;
attribute vec3    in_normal;

varying vec3      normal;

void main(void)
{
	vec4 p = gl_ModelViewProjectionMatrix * vec4(in_vertex, 1);
	gl_Position = p;
	vec4 nn = gl_ModelViewMatrixInverseTranspose * vec4(normalize(in_normal), 0);
	normal = nn.xyz;
}
