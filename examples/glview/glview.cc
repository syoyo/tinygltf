#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include <GL/glew.h>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#ifdef _WIN32
#include "../common/trackball.h"
#else
#include "trackball.h"
#endif

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef _WIN32
#include "../../tiny_gltf.h"
#else
#include "tiny_gltf.h"
#endif

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#define CheckGLErrors(desc)                                                   \
  {                                                                           \
    GLenum e = glGetError();                                                  \
    if (e != GL_NO_ERROR) {                                                   \
      printf("OpenGL error in \"%s\": %d (%d) %s:%d\n", desc, e, e, __FILE__, \
             __LINE__);                                                       \
      exit(20);                                                               \
    }                                                                         \
  }

#define CAM_Z (3.0f)
int width = 768;
int height = 768;

double prevMouseX, prevMouseY;
bool mouseLeftPressed;
bool mouseMiddlePressed;
bool mouseRightPressed;
float curr_quat[4];
float prev_quat[4];
float eye[3], lookat[3], up[3];

GLFWwindow *window;

typedef struct {
  GLuint vb;
} GLBufferState;

typedef struct {
  std::vector<GLuint> diffuseTex;  // for each primitive in mesh
} GLMeshState;

typedef struct {
  std::map<std::string, GLint> attribs;
  std::map<std::string, GLint> uniforms;
} GLProgramState;

typedef struct {
  GLuint vb;     // vertex buffer
  size_t count;  // byte count
} GLCurvesState;

std::map<int, GLBufferState> gBufferState;
std::map<std::string, GLMeshState> gMeshState;
std::map<int, GLCurvesState> gCurvesMesh;
GLProgramState gGLProgramState;

void CheckErrors(std::string desc) {
  GLenum e = glGetError();
  if (e != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error in \"%s\": %d (%d)\n", desc.c_str(), e, e);
    exit(20);
  }
}

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

bool LoadShader(GLenum shaderType,  // GL_VERTEX_SHADER or GL_FRAGMENT_SHADER(or
                                    // maybe GL_COMPUTE_SHADER)
                GLuint &shader, const char *shaderSourceFilename) {
  GLint val = 0;

  // free old shader/program
  if (shader != 0) {
    glDeleteShader(shader);
  }

  std::vector<GLchar> srcbuf;
  FILE *fp = fopen(shaderSourceFilename, "rb");
  if (!fp) {
    fprintf(stderr, "failed to load shader: %s\n", shaderSourceFilename);
    return false;
  }
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  rewind(fp);
  srcbuf.resize(len + 1);
  len = fread(&srcbuf.at(0), 1, len, fp);
  srcbuf[len] = 0;
  fclose(fp);

  const GLchar *srcs[1];
  srcs[0] = &srcbuf.at(0);

  shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, srcs, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &val);
  if (val != GL_TRUE) {
    char log[4096];
    GLsizei msglen;
    glGetShaderInfoLog(shader, 4096, &msglen, log);
    printf("%s\n", log);
    // assert(val == GL_TRUE && "failed to compile shader");
    printf("ERR: Failed to load or compile shader [ %s ]\n",
           shaderSourceFilename);
    return false;
  }

  printf("Load shader [ %s ] OK\n", shaderSourceFilename);
  return true;
}

bool LinkShader(GLuint &prog, GLuint &vertShader, GLuint &fragShader) {
  GLint val = 0;

  if (prog != 0) {
    glDeleteProgram(prog);
  }

  prog = glCreateProgram();

  glAttachShader(prog, vertShader);
  glAttachShader(prog, fragShader);
  glLinkProgram(prog);

  glGetProgramiv(prog, GL_LINK_STATUS, &val);
  assert(val == GL_TRUE && "failed to link shader");

  printf("Link shader OK\n");

  return true;
}

void reshapeFunc(GLFWwindow *window, int w, int h) {
  (void)window;
  int fb_w, fb_h;
  glfwGetFramebufferSize(window, &fb_w, &fb_h);
  glViewport(0, 0, fb_w, fb_h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, (float)w / (float)h, 0.1f, 1000.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  width = w;
  height = h;
}

void keyboardFunc(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  (void)scancode;
  (void)mods;
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    // Close window
    if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
  }
}

void clickFunc(GLFWwindow *window, int button, int action, int mods) {
  double x, y;
  glfwGetCursorPos(window, &x, &y);

  bool shiftPressed = (mods & GLFW_MOD_SHIFT);
  bool ctrlPressed = (mods & GLFW_MOD_CONTROL);

  if ((button == GLFW_MOUSE_BUTTON_LEFT) && (!shiftPressed) && (!ctrlPressed)) {
    mouseLeftPressed = true;
    mouseMiddlePressed = false;
    mouseRightPressed = false;
    if (action == GLFW_PRESS) {
      int id = -1;
      // int id = ui.Proc(x, y);
      if (id < 0) {  // outside of UI
        trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
      }
    } else if (action == GLFW_RELEASE) {
      mouseLeftPressed = false;
    }
  }
  if ((button == GLFW_MOUSE_BUTTON_RIGHT) ||
      ((button == GLFW_MOUSE_BUTTON_LEFT) && ctrlPressed)) {
    if (action == GLFW_PRESS) {
      mouseRightPressed = true;
      mouseLeftPressed = false;
      mouseMiddlePressed = false;
    } else if (action == GLFW_RELEASE) {
      mouseRightPressed = false;
    }
  }
  if ((button == GLFW_MOUSE_BUTTON_MIDDLE) ||
      ((button == GLFW_MOUSE_BUTTON_LEFT) && shiftPressed)) {
    if (action == GLFW_PRESS) {
      mouseMiddlePressed = true;
      mouseLeftPressed = false;
      mouseRightPressed = false;
    } else if (action == GLFW_RELEASE) {
      mouseMiddlePressed = false;
    }
  }
}

void motionFunc(GLFWwindow *window, double mouse_x, double mouse_y) {
  (void)window;
  float rotScale = 1.0f;
  float transScale = 2.0f;

  if (mouseLeftPressed) {
    trackball(prev_quat, rotScale * (2.0f * prevMouseX - width) / (float)width,
              rotScale * (height - 2.0f * prevMouseY) / (float)height,
              rotScale * (2.0f * mouse_x - width) / (float)width,
              rotScale * (height - 2.0f * mouse_y) / (float)height);

    add_quats(prev_quat, curr_quat, curr_quat);
  } else if (mouseMiddlePressed) {
    eye[0] += -transScale * (mouse_x - prevMouseX) / (float)width;
    lookat[0] += -transScale * (mouse_x - prevMouseX) / (float)width;
    eye[1] += transScale * (mouse_y - prevMouseY) / (float)height;
    lookat[1] += transScale * (mouse_y - prevMouseY) / (float)height;
  } else if (mouseRightPressed) {
    eye[2] += transScale * (mouse_y - prevMouseY) / (float)height;
    lookat[2] += transScale * (mouse_y - prevMouseY) / (float)height;
  }

  // Update mouse point
  prevMouseX = mouse_x;
  prevMouseY = mouse_y;
}

static size_t ComponentTypeByteSize(int type) {
  switch (type) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    case TINYGLTF_COMPONENT_TYPE_BYTE:
      return sizeof(char);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    case TINYGLTF_COMPONENT_TYPE_SHORT:
      return sizeof(short);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    case TINYGLTF_COMPONENT_TYPE_INT:
      return sizeof(int);
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
      return sizeof(float);
    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
      return sizeof(double);
    default:
      return 0;
  }
}

static void SetupMeshState(tinygltf::Model &model, GLuint progId) {
  // Buffer
  {
    for (size_t i = 0; i < model.bufferViews.size(); i++) {
      const tinygltf::BufferView &bufferView = model.bufferViews[i];
      if (bufferView.target == 0) {
        std::cout << "WARN: bufferView.target is zero" << std::endl;
        continue;  // Unsupported bufferView.
      }

      int sparse_accessor = -1;
      for (size_t a_i = 0; a_i < model.accessors.size(); ++a_i) {
        const auto &accessor = model.accessors[a_i];
        if (accessor.bufferView == i) {
          std::cout << i << " is used by accessor " << a_i << std::endl;
          if (accessor.sparse.isSparse) {
            std::cout
                << "WARN: this bufferView has at least one sparse accessor to "
                   "it. We are going to load the data as patched by this "
                   "sparse accessor, not the original data"
                << std::endl;
            sparse_accessor = a_i;
            break;
          }
        }
      }

      const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
      GLBufferState state;
      glGenBuffers(1, &state.vb);
      glBindBuffer(bufferView.target, state.vb);
      std::cout << "buffer.size= " << buffer.data.size()
                << ", byteOffset = " << bufferView.byteOffset << std::endl;

      if (sparse_accessor < 0)
        glBufferData(bufferView.target, bufferView.byteLength,
                     &buffer.data.at(0) + bufferView.byteOffset,
                     GL_STATIC_DRAW);
      else {
        const auto accessor = model.accessors[sparse_accessor];
        // copy the buffer to a temporary one for sparse patching
        unsigned char *tmp_buffer = new unsigned char[bufferView.byteLength];
        memcpy(tmp_buffer, buffer.data.data() + bufferView.byteOffset,
               bufferView.byteLength);

        const size_t size_of_object_in_buffer =
            ComponentTypeByteSize(accessor.componentType);
        const size_t size_of_sparse_indices =
            ComponentTypeByteSize(accessor.sparse.indices.componentType);

        const auto &indices_buffer_view =
            model.bufferViews[accessor.sparse.indices.bufferView];
        const auto &indices_buffer = model.buffers[indices_buffer_view.buffer];

        const auto &values_buffer_view =
            model.bufferViews[accessor.sparse.values.bufferView];
        const auto &values_buffer = model.buffers[values_buffer_view.buffer];

        for (size_t sparse_index = 0; sparse_index < accessor.sparse.count;
             ++sparse_index) {
          int index = 0;
          // std::cout << "accessor.sparse.indices.componentType = " <<
          // accessor.sparse.indices.componentType << std::endl;
          switch (accessor.sparse.indices.componentType) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
              index = (int)*(
                  unsigned char *)(indices_buffer.data.data() +
                                   indices_buffer_view.byteOffset +
                                   accessor.sparse.indices.byteOffset +
                                   (sparse_index * size_of_sparse_indices));
              break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
              index = (int)*(
                  unsigned short *)(indices_buffer.data.data() +
                                    indices_buffer_view.byteOffset +
                                    accessor.sparse.indices.byteOffset +
                                    (sparse_index * size_of_sparse_indices));
              break;
            case TINYGLTF_COMPONENT_TYPE_INT:
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
              index = (int)*(
                  unsigned int *)(indices_buffer.data.data() +
                                  indices_buffer_view.byteOffset +
                                  accessor.sparse.indices.byteOffset +
                                  (sparse_index * size_of_sparse_indices));
              break;
          }
          std::cout << "updating sparse data at index  : " << index
                    << std::endl;
          // index is now the target of the sparse index to patch in
          const unsigned char *read_from =
              values_buffer.data.data() +
              (values_buffer_view.byteOffset +
               accessor.sparse.values.byteOffset) +
              (sparse_index * (size_of_object_in_buffer * accessor.type));

          /*
          std::cout << ((float*)read_from)[0] << "\n";
          std::cout << ((float*)read_from)[1] << "\n";
          std::cout << ((float*)read_from)[2] << "\n";
          */

          unsigned char *write_to =
              tmp_buffer + index * (size_of_object_in_buffer * accessor.type);

          memcpy(write_to, read_from, size_of_object_in_buffer * accessor.type);
        }

        // debug:
        /*for(size_t p = 0; p < bufferView.byteLength/sizeof(float); p++)
        {
          float* b = (float*)tmp_buffer;
          std::cout << "modified_buffer [" << p << "] = " << b[p] << '\n';
        }*/

        glBufferData(bufferView.target, bufferView.byteLength, tmp_buffer,
                     GL_STATIC_DRAW);
        delete[] tmp_buffer;
      }
      glBindBuffer(bufferView.target, 0);

      gBufferState[i] = state;
    }
  }

#if 0  // TODO(syoyo): Implement
	// Texture
	{
		for (size_t i = 0; i < model.meshes.size(); i++) {
			const tinygltf::Mesh &mesh = model.meshes[i];

			gMeshState[mesh.name].diffuseTex.resize(mesh.primitives.size());
			for (size_t primId = 0; primId < mesh.primitives.size(); primId++) {
				const tinygltf::Primitive &primitive = mesh.primitives[primId];

				gMeshState[mesh.name].diffuseTex[primId] = 0;

				if (primitive.material < 0) {
					continue;
				}
				tinygltf::Material &mat = model.materials[primitive.material];
				// printf("material.name = %s\n", mat.name.c_str());
				if (mat.values.find("diffuse") != mat.values.end()) {
					std::string diffuseTexName = mat.values["diffuse"].string_value;
					if (model.textures.find(diffuseTexName) != model.textures.end()) {
						tinygltf::Texture &tex = model.textures[diffuseTexName];
						if (scene.images.find(tex.source) != model.images.end()) {
							tinygltf::Image &image = model.images[tex.source];
							GLuint texId;
							glGenTextures(1, &texId);
							glBindTexture(tex.target, texId);
							glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
							glTexParameterf(tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
							glTexParameterf(tex.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

							// Ignore Texture.fomat.
							GLenum format = GL_RGBA;
							if (image.component == 3) {
								format = GL_RGB;
							}
							glTexImage2D(tex.target, 0, tex.internalFormat, image.width,
									image.height, 0, format, tex.type,
									&image.image.at(0));

							CheckErrors("texImage2D");
							glBindTexture(tex.target, 0);

							printf("TexId = %d\n", texId);
							gMeshState[mesh.name].diffuseTex[primId] = texId;
						}
					}
				}
			}
		}
	}
#endif

  glUseProgram(progId);
  GLint vtloc = glGetAttribLocation(progId, "in_vertex");
  GLint nrmloc = glGetAttribLocation(progId, "in_normal");
  GLint uvloc = glGetAttribLocation(progId, "in_texcoord");

  // GLint diffuseTexLoc = glGetUniformLocation(progId, "diffuseTex");
  GLint isCurvesLoc = glGetUniformLocation(progId, "uIsCurves");

  gGLProgramState.attribs["POSITION"] = vtloc;
  gGLProgramState.attribs["NORMAL"] = nrmloc;
  gGLProgramState.attribs["TEXCOORD_0"] = uvloc;
  // gGLProgramState.uniforms["diffuseTex"] = diffuseTexLoc;
  gGLProgramState.uniforms["isCurvesLoc"] = isCurvesLoc;
};

#if 0  // TODO(syoyo): Implement
// Setup curves geometry extension
static void SetupCurvesState(tinygltf::Scene &scene, GLuint progId) {
	// Find curves primitive.
	{
		std::map<std::string, tinygltf::Mesh>::const_iterator it(
				scene.meshes.begin());
		std::map<std::string, tinygltf::Mesh>::const_iterator itEnd(
				scene.meshes.end());

		for (; it != itEnd; it++) {
			const tinygltf::Mesh &mesh = it->second;

			// Currently we only support one primitive per mesh.
			if (mesh.primitives.size() > 1) {
				continue;
			}

			for (size_t primId = 0; primId < mesh.primitives.size(); primId++) {
				const tinygltf::Primitive &primitive = mesh.primitives[primId];

				gMeshState[mesh.name].diffuseTex[primId] = 0;

				if (primitive.material.empty()) {
					continue;
				}

				bool has_curves = false;
				if (primitive.extras.IsObject()) {
					if (primitive.extras.Has("ext_mode")) {
						const tinygltf::Value::Object &o =
							primitive.extras.Get<tinygltf::Value::Object>();
						const tinygltf::Value &ext_mode = o.find("ext_mode")->second;

						if (ext_mode.IsString()) {
							const std::string &str = ext_mode.Get<std::string>();
							if (str.compare("curves") == 0) {
								has_curves = true;
							}
						}
					}
				}

				if (!has_curves) {
					continue;
				}

				// Construct curves buffer
				const tinygltf::Accessor &vtx_accessor =
					scene.accessors[primitive.attributes.find("POSITION")->second];
				const tinygltf::Accessor &nverts_accessor =
					scene.accessors[primitive.attributes.find("NVERTS")->second];
				const tinygltf::BufferView &vtx_bufferView =
					scene.bufferViews[vtx_accessor.bufferView];
				const tinygltf::BufferView &nverts_bufferView =
					scene.bufferViews[nverts_accessor.bufferView];
				const tinygltf::Buffer &vtx_buffer =
					scene.buffers[vtx_bufferView.buffer];
				const tinygltf::Buffer &nverts_buffer =
					scene.buffers[nverts_bufferView.buffer];

				// std::cout << "vtx_bufferView = " << vtx_accessor.bufferView <<
				// std::endl;
				// std::cout << "nverts_bufferView = " << nverts_accessor.bufferView <<
				// std::endl;
				// std::cout << "vtx_buffer.size = " << vtx_buffer.data.size() <<
				// std::endl;
				// std::cout << "nverts_buffer.size = " << nverts_buffer.data.size() <<
				// std::endl;

				const int *nverts =
					reinterpret_cast<const int *>(nverts_buffer.data.data());
				const float *vtx =
					reinterpret_cast<const float *>(vtx_buffer.data.data());

				// Convert to GL_LINES data.
				std::vector<float> line_pts;
				size_t vtx_offset = 0;
				for (int k = 0; k < static_cast<int>(nverts_accessor.count); k++) {
					for (int n = 0; n < nverts[k] - 1; n++) {

						line_pts.push_back(vtx[3 * (vtx_offset + n) + 0]);
						line_pts.push_back(vtx[3 * (vtx_offset + n) + 1]);
						line_pts.push_back(vtx[3 * (vtx_offset + n) + 2]);

						line_pts.push_back(vtx[3 * (vtx_offset + n + 1) + 0]);
						line_pts.push_back(vtx[3 * (vtx_offset + n + 1) + 1]);
						line_pts.push_back(vtx[3 * (vtx_offset + n + 1) + 2]);

						// std::cout << "p0 " << vtx[3 * (vtx_offset + n) + 0] << ", "
						//                  << vtx[3 * (vtx_offset + n) + 1] << ", "
						//                  << vtx[3 * (vtx_offset + n) + 2] << std::endl;

						// std::cout << "p1 " << vtx[3 * (vtx_offset + n+1) + 0] << ", "
						//                  << vtx[3 * (vtx_offset + n+1) + 1] << ", "
						//                  << vtx[3 * (vtx_offset + n+1) + 2] << std::endl;
					}

					vtx_offset += nverts[k];
				}

				GLCurvesState state;
				glGenBuffers(1, &state.vb);
				glBindBuffer(GL_ARRAY_BUFFER, state.vb);
				glBufferData(GL_ARRAY_BUFFER, line_pts.size() * sizeof(float),
						line_pts.data(), GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				state.count = line_pts.size() / 3;
				gCurvesMesh[mesh.name] = state;

				// Material
				tinygltf::Material &mat = scene.materials[primitive.material];
				// printf("material.name = %s\n", mat.name.c_str());
				if (mat.values.find("diffuse") != mat.values.end()) {
					std::string diffuseTexName = mat.values["diffuse"].string_value;
					if (scene.textures.find(diffuseTexName) != scene.textures.end()) {
						tinygltf::Texture &tex = scene.textures[diffuseTexName];
						if (scene.images.find(tex.source) != scene.images.end()) {
							tinygltf::Image &image = scene.images[tex.source];
							GLuint texId;
							glGenTextures(1, &texId);
							glBindTexture(tex.target, texId);
							glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
							glTexParameterf(tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
							glTexParameterf(tex.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

							// Ignore Texture.fomat.
							GLenum format = GL_RGBA;
							if (image.component == 3) {
								format = GL_RGB;
							}
							glTexImage2D(tex.target, 0, tex.internalFormat, image.width,
									image.height, 0, format, tex.type,
									&image.image.at(0));

							CheckErrors("texImage2D");
							glBindTexture(tex.target, 0);

							printf("TexId = %d\n", texId);
							gMeshState[mesh.name].diffuseTex[primId] = texId;
						}
					}
				}
			}
		}
	}

	glUseProgram(progId);
	GLint vtloc = glGetAttribLocation(progId, "in_vertex");
	GLint nrmloc = glGetAttribLocation(progId, "in_normal");
	GLint uvloc = glGetAttribLocation(progId, "in_texcoord");

	GLint diffuseTexLoc = glGetUniformLocation(progId, "diffuseTex");
	GLint isCurvesLoc = glGetUniformLocation(progId, "uIsCurves");

	gGLProgramState.attribs["POSITION"] = vtloc;
	gGLProgramState.attribs["NORMAL"] = nrmloc;
	gGLProgramState.attribs["TEXCOORD_0"] = uvloc;
	gGLProgramState.uniforms["diffuseTex"] = diffuseTexLoc;
	gGLProgramState.uniforms["uIsCurves"] = isCurvesLoc;
};
#endif

static void DrawMesh(tinygltf::Model &model, const tinygltf::Mesh &mesh) {
  //// Skip curves primitive.
  // if (gCurvesMesh.find(mesh.name) != gCurvesMesh.end()) {
  //  return;
  //}

  // if (gGLProgramState.uniforms["diffuseTex"] >= 0) {
  //  glUniform1i(gGLProgramState.uniforms["diffuseTex"], 0);  // TEXTURE0
  //}

  if (gGLProgramState.uniforms["isCurvesLoc"] >= 0) {
    glUniform1i(gGLProgramState.uniforms["isCurvesLoc"], 0);
  }

  for (size_t i = 0; i < mesh.primitives.size(); i++) {
    const tinygltf::Primitive &primitive = mesh.primitives[i];

    if (primitive.indices < 0) return;

    // Assume TEXTURE_2D target for the texture object.
    // glBindTexture(GL_TEXTURE_2D, gMeshState[mesh.name].diffuseTex[i]);

    std::map<std::string, int>::const_iterator it(primitive.attributes.begin());
    std::map<std::string, int>::const_iterator itEnd(
        primitive.attributes.end());

    for (; it != itEnd; it++) {
      assert(it->second >= 0);
      const tinygltf::Accessor &accessor = model.accessors[it->second];
      glBindBuffer(GL_ARRAY_BUFFER, gBufferState[accessor.bufferView].vb);
      CheckErrors("bind buffer");
      int size = 1;
      if (accessor.type == TINYGLTF_TYPE_SCALAR) {
        size = 1;
      } else if (accessor.type == TINYGLTF_TYPE_VEC2) {
        size = 2;
      } else if (accessor.type == TINYGLTF_TYPE_VEC3) {
        size = 3;
      } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
        size = 4;
      } else {
        assert(0);
      }
      // it->first would be "POSITION", "NORMAL", "TEXCOORD_0", ...
      if ((it->first.compare("POSITION") == 0) ||
          (it->first.compare("NORMAL") == 0) ||
          (it->first.compare("TEXCOORD_0") == 0)) {
        if (gGLProgramState.attribs[it->first] >= 0) {
          // Compute byteStride from Accessor + BufferView combination.
          int byteStride =
              accessor.ByteStride(model.bufferViews[accessor.bufferView]);
          assert(byteStride != -1);
          glVertexAttribPointer(gGLProgramState.attribs[it->first], size,
                                accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byteStride, BUFFER_OFFSET(accessor.byteOffset));
          CheckErrors("vertex attrib pointer");
          glEnableVertexAttribArray(gGLProgramState.attribs[it->first]);
          CheckErrors("enable vertex attrib array");
        }
      }
    }

    const tinygltf::Accessor &indexAccessor =
        model.accessors[primitive.indices];
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                 gBufferState[indexAccessor.bufferView].vb);
    CheckErrors("bind buffer");
    int mode = -1;
    if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
      mode = GL_TRIANGLES;
    } else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
      mode = GL_TRIANGLE_STRIP;
    } else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) {
      mode = GL_TRIANGLE_FAN;
    } else if (primitive.mode == TINYGLTF_MODE_POINTS) {
      mode = GL_POINTS;
    } else if (primitive.mode == TINYGLTF_MODE_LINE) {
      mode = GL_LINES;
    } else if (primitive.mode == TINYGLTF_MODE_LINE_LOOP) {
      mode = GL_LINE_LOOP;
    } else {
      assert(0);
    }
    glDrawElements(mode, indexAccessor.count, indexAccessor.componentType,
                   BUFFER_OFFSET(indexAccessor.byteOffset));
    CheckErrors("draw elements");

    {
      std::map<std::string, int>::const_iterator it(
          primitive.attributes.begin());
      std::map<std::string, int>::const_iterator itEnd(
          primitive.attributes.end());

      for (; it != itEnd; it++) {
        if ((it->first.compare("POSITION") == 0) ||
            (it->first.compare("NORMAL") == 0) ||
            (it->first.compare("TEXCOORD_0") == 0)) {
          if (gGLProgramState.attribs[it->first] >= 0) {
            glDisableVertexAttribArray(gGLProgramState.attribs[it->first]);
          }
        }
      }
    }
  }
}

#if 0  // TODO(syoyo): Implement
static void DrawCurves(tinygltf::Scene &scene, const tinygltf::Mesh &mesh) {
	(void)scene;

	if (gCurvesMesh.find(mesh.name) == gCurvesMesh.end()) {
		return;
	}

	if (gGLProgramState.uniforms["isCurvesLoc"] >= 0) {
		glUniform1i(gGLProgramState.uniforms["isCurvesLoc"], 1);
	}

	GLCurvesState &state = gCurvesMesh[mesh.name];

	if (gGLProgramState.attribs["POSITION"] >= 0) {
		glBindBuffer(GL_ARRAY_BUFFER, state.vb);
		glVertexAttribPointer(gGLProgramState.attribs["POSITION"], 3, GL_FLOAT,
				GL_FALSE, /* stride */ 0, BUFFER_OFFSET(0));
		CheckErrors("curve: vertex attrib pointer");
		glEnableVertexAttribArray(gGLProgramState.attribs["POSITION"]);
		CheckErrors("curve: enable vertex attrib array");
	}

	glDrawArrays(GL_LINES, 0, state.count);

	if (gGLProgramState.attribs["POSITION"] >= 0) {
		glDisableVertexAttribArray(gGLProgramState.attribs["POSITION"]);
	}
}
#endif

// Hierarchically draw nodes
static void DrawNode(tinygltf::Model &model, const tinygltf::Node &node) {
  // Apply xform

  glPushMatrix();
  if (node.matrix.size() == 16) {
    // Use `matrix' attribute
    glMultMatrixd(node.matrix.data());
  } else {
    // Assume Trans x Rotate x Scale order
    if (node.scale.size() == 3) {
      glScaled(node.scale[0], node.scale[1], node.scale[2]);
    }

    if (node.rotation.size() == 4) {
      glRotated(node.rotation[0], node.rotation[1], node.rotation[2],
                node.rotation[3]);
    }

    if (node.translation.size() == 3) {
      glTranslated(node.translation[0], node.translation[1],
                   node.translation[2]);
    }
  }

  // std::cout << "node " << node.name << ", Meshes " << node.meshes.size() <<
  // std::endl;

  // std::cout << it->first << std::endl;
  // FIXME(syoyo): Refactor.
  // DrawCurves(scene, it->second);
  if (node.mesh > -1) {
    assert(node.mesh < model.meshes.size());
    DrawMesh(model, model.meshes[node.mesh]);
  }

  // Draw child nodes.
  for (size_t i = 0; i < node.children.size(); i++) {
    assert(node.children[i] < model.nodes.size());
    DrawNode(model, model.nodes[node.children[i]]);
  }

  glPopMatrix();
}

static void DrawModel(tinygltf::Model &model) {
#if 0
	std::map<std::string, tinygltf::Mesh>::const_iterator it(scene.meshes.begin());
	std::map<std::string, tinygltf::Mesh>::const_iterator itEnd(scene.meshes.end());

	for (; it != itEnd; it++) {
		DrawMesh(scene, it->second);
		DrawCurves(scene, it->second);
	}
#else
  // If the glTF asset has at least one scene, and doesn't define a default one
  // just show the first one we can find
  assert(model.scenes.size() > 0);
  int scene_to_display = model.defaultScene > -1 ? model.defaultScene : 0;
  const tinygltf::Scene &scene = model.scenes[scene_to_display];
  for (size_t i = 0; i < scene.nodes.size(); i++) {
    DrawNode(model, model.nodes[scene.nodes[i]]);
  }
#endif
}

static void Init() {
  trackball(curr_quat, 0, 0, 0, 0);

  eye[0] = 0.0f;
  eye[1] = 0.0f;
  eye[2] = CAM_Z;

  lookat[0] = 0.0f;
  lookat[1] = 0.0f;
  lookat[2] = 0.0f;

  up[0] = 0.0f;
  up[1] = 1.0f;
  up[2] = 0.0f;
}

static void PrintNodes(const tinygltf::Scene &scene) {
  for (size_t i = 0; i < scene.nodes.size(); i++) {
    std::cout << "node.name : " << scene.nodes[i] << std::endl;
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "glview input.gltf <scale>" << std::endl;
    std::cout << "defaulting to example cube model" << std::endl;
  }

  float scale = 1.0f;
  if (argc > 2) {
    scale = atof(argv[2]);
  }

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

#ifdef _WIN32
#ifdef _DEBUG
  std::string input_filename(argv[1] ? argv[1]
                                     : "../../../models/Cube/Cube.gltf");
#endif
#else
  std::string input_filename(argv[1] ? argv[1] : "../../models/Cube/Cube.gltf");
#endif

  std::string ext = GetFilePathExtension(input_filename);

  bool ret = false;
  if (ext.compare("glb") == 0) {
    // assume binary glTF.
    ret =
        loader.LoadBinaryFromFile(&model, &err, &warn, input_filename.c_str());
  } else {
    // assume ascii glTF.
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
  }

  if (!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
  }

  if (!err.empty()) {
    printf("ERR: %s\n", err.c_str());
  }
  if (!ret) {
    printf("Failed to load .glTF : %s\n", argv[1]);
    exit(-1);
  }

  Init();

  // DBG
  PrintNodes(model.scenes[model.defaultScene > -1 ? model.defaultScene : 0]);

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return -1;
  }

  std::stringstream ss;
  ss << "Simple glTF viewer: " << input_filename;

  std::string title = ss.str();

  window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
  if (window == NULL) {
    std::cerr << "Failed to open GLFW window. " << std::endl;
    glfwTerminate();
    return 1;
  }

  glfwGetWindowSize(window, &width, &height);

  glfwMakeContextCurrent(window);

  // Callback
  glfwSetWindowSizeCallback(window, reshapeFunc);
  glfwSetKeyCallback(window, keyboardFunc);
  glfwSetMouseButtonCallback(window, clickFunc);
  glfwSetCursorPosCallback(window, motionFunc);

  glewExperimental = true;  // This may be only true for linux environment.
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW." << std::endl;
    return -1;
  }

  reshapeFunc(window, width, height);

  GLuint vertId = 0, fragId = 0, progId = 0;

#ifdef _WIN32
#ifdef _DEBUG
  const char *shader_frag_filename = "../shader.frag";
  const char *shader_vert_filename = "../shader.vert";
#endif
#else
  const char *shader_frag_filename = "shader.frag";
  const char *shader_vert_filename = "shader.vert";
#endif

  if (false == LoadShader(GL_VERTEX_SHADER, vertId, shader_vert_filename)) {
    return -1;
  }
  CheckErrors("load vert shader");

  if (false == LoadShader(GL_FRAGMENT_SHADER, fragId, shader_frag_filename)) {
    return -1;
  }
  CheckErrors("load frag shader");

  if (false == LinkShader(progId, vertId, fragId)) {
    return -1;
  }

  CheckErrors("link");

  {
    // At least `in_vertex` should be used in the shader.
    GLint vtxLoc = glGetAttribLocation(progId, "in_vertex");
    if (vtxLoc < 0) {
      printf("vertex loc not found.\n");
      exit(-1);
    }
  }

  glUseProgram(progId);
  CheckErrors("useProgram");

  SetupMeshState(model, progId);
  // SetupCurvesState(model, progId);
  CheckErrors("SetupGLState");

  std::cout << "# of meshes = " << model.meshes.size() << std::endl;

  while (glfwWindowShouldClose(window) == GL_FALSE) {
    glfwPollEvents();
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    GLfloat mat[4][4];
    build_rotmatrix(mat, curr_quat);

    // camera(define it in projection matrix)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0],
              up[1], up[2]);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(&mat[0][0]);

    glScalef(scale, scale, scale);

    DrawModel(model);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glFlush();

    glfwSwapBuffers(window);
  }

  glfwTerminate();
}
