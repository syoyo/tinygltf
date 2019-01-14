#include "shaders.h"
#include <iostream>
#include <vector>


#include <GL/glew.h>
#include <GLFW/glfw3.h>

std::string FragmentShaderCode =
"#version 330 core\n\
in vec3 normal;\n\
in vec3 position;\n\
in vec2 texcoord;\n\
\n\
uniform sampler2D tex;\n\
uniform vec3 sun_position; \n\
uniform vec3 sun_color; \n\
\n\
out vec4 color;\n\
void main() {\n\
	float lum = max(dot(normal, normalize(sun_position)), 0.0);\n\
	color = texture(tex, texcoord) * vec4((0.3 + 0.7 * lum) * sun_color, 1.0);\n\
}\n\
";

std::string VertexShaderCode =
"#version 330 core\n\
layout(location = 0) in vec3 in_vertex;\n\
layout(location = 1) in vec3 in_normal;\n\
layout(location = 2) in vec2 in_texcoord;\n\
\n\
uniform mat4 MVP;\n\
\n\
out vec3 normal;\n\
out vec3 position;\n\
out vec2 texcoord;\n\
\n\
void main(){\n\
	gl_Position = MVP * vec4(in_vertex, 1);\n\
	position = gl_Position.xyz;\n\
	normal = normalize(mat3(MVP) * in_normal);\n\
	position = in_vertex;\n\
	texcoord = in_texcoord;\n\
}";


Shaders::Shaders()
{

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);


	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	this->pid = ProgramID;
}


Shaders::~Shaders()
{
}
