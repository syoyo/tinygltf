#include <fstream>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "shaders.h"
#include "window.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

// Inlude tinyktx.h before tiny_gltf.h
// to get TKTX_*** definitions
#define TINYKTX_IMPLEMENTATION
#include "../../tinyktx.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#define TINYGLTF_ENABLE_KTX
// tinyktx.h is already included above,
// so let tiny_gltf.h know do not include tinyktx.h anymore
#define TINYGLTF_NO_INCLUDE_TINY_KTX
#include "../../tiny_gltf.h"

//#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
#define BUFFER_OFFSET(i) \
  (reinterpret_cast<void *>(i))  // TODO(syoyo): Is this right way?

bool loadModel(tinygltf::Model &model, const char *filename) {
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cout << "ERR: " << err << std::endl;
  }

  if (!res)
    std::cout << "Failed to load glTF: " << filename << std::endl;
  else
    std::cout << "Loaded glTF: " << filename << std::endl;

  return res;
}

static bool GetOpenGLFormatFromKTX(int ktx_fmt, GLint *internal_format, GLenum *format, GLenum *type) {
#if defined(TINYGLTF_ENABLE_KTX)
    bool ret = true;

    std::cout << "fmt = " << ktx_fmt << ", rgb8 fmt = " << TKTX_R8G8B8_UNORM << "\n";

    if (ktx_fmt == TKTX_R8G8B8_UNORM) {
      (*internal_format) = GL_RGB;
      (*format) = GL_RGB;
      (*type) = GL_UNSIGNED_BYTE;
    } else {
      // TODO(syoyo): Support more KTX formats.
      ret = false;
    }

    return ret;
#else
    (void)fmt;
    (void)internal_format;
    (void)internal_format;
    return false;
#endif
}

std::map<int, GLuint> bindMesh(std::map<int, GLuint> vbos,
                               tinygltf::Model &model, tinygltf::Mesh &mesh) {
  for (size_t i = 0; i < model.bufferViews.size(); ++i) {
    const tinygltf::BufferView &bufferView = model.bufferViews[i];
    if (bufferView.target == 0) {  // TODO impl drawarrays
      std::cout << "WARN: bufferView.target is zero" << std::endl;
      continue;  // Unsupported bufferView.
                 /*
                   From spec2.0 readme:
                   https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
                            ... drawArrays function should be used with a count equal to
                   the count            property of any of the accessors referenced by the
                   attributes            property            (they are all equal for a
                   given            primitive).
                 */
    }

    tinygltf::Buffer buffer = model.buffers[bufferView.buffer];
    std::cout << "bufferview.target " << bufferView.target << std::endl;

    GLuint vbo;
    glGenBuffers(1, &vbo);
    vbos[i] = vbo;
    glBindBuffer(bufferView.target, vbo);

    std::cout << "buffer.data.size = " << buffer.data.size()
              << ", bufferview.byteOffset = " << bufferView.byteOffset
              << std::endl;

    glBufferData(bufferView.target, bufferView.byteLength,
                 &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
  }

  for (size_t i = 0; i < mesh.primitives.size(); ++i) {
    tinygltf::Primitive primitive = mesh.primitives[i];
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

    for (auto &attrib : primitive.attributes) {
      tinygltf::Accessor accessor = model.accessors[attrib.second];
      int byteStride =
          accessor.ByteStride(model.bufferViews[accessor.bufferView]);
      glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);

      int size = 1;
      if (accessor.type != TINYGLTF_TYPE_SCALAR) {
        size = accessor.type;
      }

      int vaa = -1;
      if (attrib.first.compare("POSITION") == 0) vaa = 0;
      if (attrib.first.compare("NORMAL") == 0) vaa = 1;
      if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
      if (vaa > -1) {
        glEnableVertexAttribArray(vaa);
        glVertexAttribPointer(vaa, size, accessor.componentType,
                              accessor.normalized ? GL_TRUE : GL_FALSE,
                              byteStride, BUFFER_OFFSET(accessor.byteOffset));
      } else
        std::cout << "Unsupported vertex attribute: " << attrib.first << std::endl;
    }

    GLuint texid;
    glGenTextures(1, &texid);

    tinygltf::Texture &tex = model.textures[0];
    tinygltf::Image &image = model.images[tex.source];

    glBindTexture(GL_TEXTURE_2D, texid);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLint internal_format = GL_RGBA;
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;

    bool valid = false;

    // KTX extension
    if (image.extras.Has("ktx_format")) {

      valid = GetOpenGLFormatFromKTX(image.extras.Get("ktx_format").Get<int>(), &internal_format, &format, &type);

      if (valid) {

        std::cout << "ktx_format: " << image.extras.Get("ktx_format").Get<int>() << std::endl;
        std::cout << "ktx image size: " << image.width << ", " << image.height << std::endl;
      }

    } else {

      if (image.component == 1) {
        format = GL_RED;
      } else if (image.component == 2) {
        format = GL_RG;
      } else if (image.component == 3) {
        format = GL_RGB;
      } else if (image.component == 4) {
        format = GL_RGBA;
      } else {
        valid = false;
      }

      if (image.bits == 8) {
        // ok
      } else if (image.bits == 16) {
        type = GL_UNSIGNED_SHORT;
      } else {
        valid = false;
      }

    }

    if (valid) {

      glTexImage2D(GL_TEXTURE_2D, 0, internal_format, image.width, image.height, 0,
                   format, type, &image.image.at(0));
    }
  }

  return vbos;
}

// bind models
void bindModelNodes(std::map<int, GLuint> vbos, tinygltf::Model &model,
                    tinygltf::Node &node) {
  bindMesh(vbos, model, model.meshes[node.mesh]);
  for (size_t i = 0; i < node.children.size(); i++) {
    bindModelNodes(vbos, model, model.nodes[node.children[i]]);
  }
}
GLuint bindModel(tinygltf::Model &model) {
  std::map<int, GLuint> vbos;
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  const tinygltf::Scene &scene = model.scenes[model.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    bindModelNodes(vbos, model, model.nodes[scene.nodes[i]]);
  }

  glBindVertexArray(0);
  // cleanup vbos
  for (size_t i = 0; i < vbos.size(); ++i) {
    glDeleteBuffers(1, &vbos[i]);
  }

  return vao;
}

void drawMesh(tinygltf::Model &model, tinygltf::Mesh &mesh) {
  for (size_t i = 0; i < mesh.primitives.size(); ++i) {
    tinygltf::Primitive primitive = mesh.primitives[i];
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

    glDrawElements(primitive.mode, indexAccessor.count,
                   indexAccessor.componentType,
                   BUFFER_OFFSET(indexAccessor.byteOffset));
  }
}

// recursively draw node and children nodes of model
void drawModelNodes(tinygltf::Model &model, tinygltf::Node &node) {
  drawMesh(model, model.meshes[node.mesh]);
  for (size_t i = 0; i < node.children.size(); i++) {
    drawModelNodes(model, model.nodes[node.children[i]]);
  }
}
void drawModel(GLuint vao, tinygltf::Model &model) {
  glBindVertexArray(vao);

  const tinygltf::Scene &scene = model.scenes[model.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    drawModelNodes(model, model.nodes[scene.nodes[i]]);
  }

  glBindVertexArray(0);
}

void dbgModel(tinygltf::Model &model) {
  for (auto &mesh : model.meshes) {
    std::cout << "mesh : " << mesh.name << std::endl;
    for (auto &primitive : mesh.primitives) {
      const tinygltf::Accessor &indexAccessor =
          model.accessors[primitive.indices];

      std::cout << "indexaccessor: count " << indexAccessor.count << ", type "
                << indexAccessor.componentType << std::endl;

      tinygltf::Material &mat = model.materials[primitive.material];
      for (auto &mats : mat.values) {
        std::cout << "mat : " << mats.first.c_str() << std::endl;
      }

      for (auto &image : model.images) {
        std::cout << "image name : " << image.uri << std::endl;
        std::cout << "  size : " << image.image.size() << std::endl;
        std::cout << "  w/h : " << image.width << "/" << image.height
                  << std::endl;
      }

      std::cout << "indices : " << primitive.indices << std::endl;
      std::cout << "mode     : "
                << "(" << primitive.mode << ")" << std::endl;

      for (auto &attrib : primitive.attributes) {
        std::cout << "attribute : " << attrib.first.c_str() << std::endl;
      }
    }
  }
}

glm::mat4 genView(glm::vec3 pos, glm::vec3 lookat) {
  // Camera matrix
  glm::mat4 view = glm::lookAt(
      pos,                // Camera in World Space
      lookat,             // and looks at the origin
      glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
  );

  return view;
}

glm::mat4 genMVP(glm::mat4 view_mat, glm::mat4 model_mat, float fov, int w,
                 int h) {
  glm::mat4 Projection =
      glm::perspective(glm::radians(fov), (float)w / (float)h, 0.01f, 1000.0f);

  // Or, for an ortho camera :
  // glm::mat4 Projection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f);
  // // In world coordinates

  glm::mat4 mvp = Projection * view_mat * model_mat;

  return mvp;
}

void displayLoop(Window &window, const std::string &filename) {
  Shaders shader = Shaders();
  glUseProgram(shader.pid);

  // grab uniforms to modify
  GLuint MVP_u = glGetUniformLocation(shader.pid, "MVP");
  GLuint sun_position_u = glGetUniformLocation(shader.pid, "sun_position");
  GLuint sun_color_u = glGetUniformLocation(shader.pid, "sun_color");

  tinygltf::Model model;
  if (!loadModel(model, filename.c_str())) return;

  GLuint vao = bindModel(model);
  // dbgModel(model); return;

  // Model matrix : an identity matrix (model will be at the origin)
  glm::mat4 model_mat = glm::mat4(1.0f);
  glm::mat4 model_rot = glm::mat4(1.0f);
  glm::vec3 model_pos = glm::vec3(-3, 0, -3);

  // generate a camera view, based on eye-position and lookAt world-position
  glm::mat4 view_mat = genView(glm::vec3(2, 2, 20), model_pos);

  glm::vec3 sun_position = glm::vec3(3.0, 10.0, -5.0);
  glm::vec3 sun_color = glm::vec3(1.0);

  while (!window.Close()) {
    window.Resize();

    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 trans =
        glm::translate(glm::mat4(1.0f), model_pos);  // reposition model
    model_rot = glm::rotate(model_rot, glm::radians(0.8f),
                            glm::vec3(0, 1, 0));  // rotate model on y axis
    model_mat = trans * model_rot;

    // build a model-view-projection
    GLint w, h;
    glfwGetWindowSize(window.window, &w, &h);
    glm::mat4 mvp = genMVP(view_mat, model_mat, 45.0f, w, h);
    glUniformMatrix4fv(MVP_u, 1, GL_FALSE, &mvp[0][0]);

    glUniform3fv(sun_position_u, 1, &sun_position[0]);
    glUniform3fv(sun_color_u, 1, &sun_color[0]);

    drawModel(vao, model);
    glfwSwapBuffers(window.window);
    glfwPollEvents();
  }
}

static void error_callback(int error, const char *description) {
  (void)error;
  fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char **argv) {
  std::string filename = "../../models/Cube/Cube.gltf";

  if (argc > 1) {
    filename = argv[1];
  }

  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) return -1;

    // NOTE(syoyo): For some reason, Linux + NVIDIA driver + apt-installed
    // glew(1.13) cannot initialize some ARB functions when CONTEXT_VERSION are
    // explicitly given. Proably we need to compile app with recent glfw and
    // glew(or use glad) package
#if !defined(__linux__)
  // Try to create OpenGL 3.3 context on Windows and macOS
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  Window window = Window(800, 600, "TinyGLTF basic example");
  glfwMakeContextCurrent(window.window);

#ifdef __APPLE__
  // https://stackoverflow.com/questions/50192625/openggl-segmentation-fault
  glewExperimental = GL_TRUE;
#endif

  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialie glew." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << glGetString(GL_RENDERER) << ", " << glGetString(GL_VERSION)
            << std::endl;

  if (!GLEW_ARB_vertex_array_object) {
    std::cerr << "GLEW_ARB_vertex_array_object was not available." << std::endl;
    return EXIT_FAILURE;
  }

  if (!GLEW_VERSION_3_3) {
    std::cerr << "OpenGL 3.3 is required to execute this app." << std::endl;
    return EXIT_FAILURE;
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  displayLoop(window, filename);

  glfwTerminate();
  return 0;
}
