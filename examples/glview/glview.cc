#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <cassert>
#include <cmath>

#include <GL/glew.h>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <GLFW/glfw3.h>

#include "trackball.h"

#define TINYGLTF_LOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf_loader.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

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

GLFWwindow* window;

typedef struct
{
  GLuint vb;
} GLBufferState;

typedef struct
{
  std::map<std::string, GLint> attribs;
} GLProgramState;

std::map<std::string, GLBufferState> gBufferState;
GLProgramState gGLProgramState;

void CheckErrors(std::string desc) {
  GLenum e = glGetError();
  if (e != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error in \"%s\": %d (%d)\n", desc.c_str(), e, e);
    exit(20);
  }
}

bool LoadShader(GLenum shaderType, // GL_VERTEX_SHADER or GL_FRAGMENT_SHADER(or
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

  const GLchar* srcs[1];
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

void reshapeFunc(GLFWwindow* window, int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, (float)w / (float)h, 0.1f, 1000.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  width = w;
  height = h;
}

void keyboardFunc(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if(action == GLFW_PRESS || action == GLFW_REPEAT){
    // Close window
    if(key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

void clickFunc(GLFWwindow* window, int button, int action, int mods){
    double x, y;
    glfwGetCursorPos(window,&x, &y);

    if(button == GLFW_MOUSE_BUTTON_LEFT){
        mouseLeftPressed = true;
        if(action == GLFW_PRESS){
          int id = -1;
          //int id = ui.Proc(x, y);
          if (id < 0) { // outside of UI 
            trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
          }
        } else if(action == GLFW_RELEASE){
            mouseLeftPressed = false;
        }
  }
  if(button == GLFW_MOUSE_BUTTON_RIGHT){
    if(action == GLFW_PRESS){
      mouseRightPressed = true;
    } else if(action == GLFW_RELEASE){
      mouseRightPressed = false;
    }
  }
  if(button == GLFW_MOUSE_BUTTON_MIDDLE){
    if(action == GLFW_PRESS){
      mouseMiddlePressed = true;
    } else if(action == GLFW_RELEASE){
      mouseMiddlePressed = false;
    }
  }
}

void motionFunc(GLFWwindow* window, double mouse_x, double mouse_y){
  float rotScale = 1.0f;
  float transScale = 2.0f;

    if(mouseLeftPressed){
      trackball(prev_quat,
          rotScale * (2.0f * prevMouseX - width) / (float)width,
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

static void SetupGLState(Scene& scene, GLuint progId)
{
  std::map<std::string, BufferView>::const_iterator it(scene.bufferViews.begin());
  std::map<std::string, BufferView>::const_iterator itEnd(scene.bufferViews.end());

  for (; it != itEnd; it++) {
    const BufferView& bufferView = it->second;
    if (bufferView.target == 0) {
      continue; // Unsupported bufferView.
    }

    const Buffer& buffer = scene.buffers[bufferView.buffer];
    GLBufferState state;
    glGenBuffers(1, &state.vb);
    glBindBuffer(bufferView.target, state.vb);
    glBufferData(bufferView.target, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
    glBindBuffer(bufferView.target, 0);
    
    gBufferState[it->first] = state;
  }

  glUseProgram(progId);
  GLint vtloc = glGetAttribLocation(progId, "in_vertex");
  GLint nrmloc = glGetAttribLocation(progId, "in_normal");
  gGLProgramState.attribs["POSITION"] = vtloc;
  gGLProgramState.attribs["NORMAL"] = nrmloc;
  
};

void DrawMesh(Scene& scene, const Mesh& mesh)
{
  for (size_t i = 0; i < mesh.primitives.size(); i++) { 
    const Primitive& primitive = mesh.primitives[i];

    if (primitive.indices.empty()) return;

    std::map<std::string, std::string>::const_iterator it(primitive.attributes.begin());
    std::map<std::string, std::string>::const_iterator itEnd(primitive.attributes.end());

    for (; it != itEnd; it++) {
      const Accessor& accessor = scene.accessors[it->second];
      glBindBuffer(GL_ARRAY_BUFFER, gBufferState[accessor.bufferView].vb);
      CheckErrors("bind buffer");
      int count = 1;
      if (accessor.type == TINYGLTF_TYPE_SCALAR) {
        count = 1;
      } else if (accessor.type == TINYGLTF_TYPE_VEC2) {
        count = 2;
      } else if (accessor.type == TINYGLTF_TYPE_VEC3) {
        count = 3;
      } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
        count = 4;
      }
      // it->first would be "POSITION", "NORMAL", ...
      if ( (it->first.compare("POSITION") == 0) ||
           (it->first.compare("NORMAL") == 0)) {
        glVertexAttribPointer(gGLProgramState.attribs[it->first], count, accessor.componentType, GL_FALSE, accessor.byteStride, BUFFER_OFFSET(accessor.byteOffset));
        CheckErrors("vertex attrib pointer");
        glEnableVertexAttribArray(gGLProgramState.attribs[it->first]);
        CheckErrors("enable vertex attrib array");
      }
    }

    const Accessor& indexAccessor = scene.accessors[primitive.indices];
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gBufferState[indexAccessor.bufferView].vb);
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
    };
    glDrawElements(mode, indexAccessor.count, indexAccessor.componentType, BUFFER_OFFSET(indexAccessor.byteOffset));
    CheckErrors("draw elements");

    {
      std::map<std::string, std::string>::const_iterator it(primitive.attributes.begin());
      std::map<std::string, std::string>::const_iterator itEnd(primitive.attributes.end());

      for (; it != itEnd; it++) {
        if ( (it->first.compare("POSITION") == 0) ||
             (it->first.compare("NORMAL") == 0)) {
          glDisableVertexAttribArray(gGLProgramState.attribs[it->first]);
        }
      }
    }
  }

}

void DrawScene(Scene& scene)
{
  std::map<std::string, Mesh>::const_iterator it(scene.meshes.begin());
  std::map<std::string, Mesh>::const_iterator itEnd(scene.meshes.end());

  for (; it != itEnd; it++) {
    DrawMesh(scene, it->second);
  }
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

int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cout << "glview input.gltf <scale>\n" << std::endl;
    return 0;
  }

  float scale = 1.0f;
  if (argc > 2) {
    scale = atof(argv[2]);
  }

  Scene scene; 
  TinyGLTFLoader loader;
  std::string err;
  
  bool ret = loader.LoadFromFile(scene, err, argv[1]);
  if (!err.empty()) {
    printf("ERR: %s\n", err.c_str());
  }
  if (!ret) {
    printf("Failed to load .glTF : %s\n", argv[1]);
    exit(-1);
  }

  Init();

  if(!glfwInit()){
    std::cerr << "Failed to initialize GLFW." << std::endl;
    return -1;
  }

  window = glfwCreateWindow(width, height, "Simple glTF geometry viewer", NULL, NULL);
  if(window == NULL){
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

  glewExperimental = true;
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW." << std::endl;
    return -1;
  }

  reshapeFunc(window, width, height);

  GLuint vertId = 0, fragId = 0, progId = 0;
  if (false == LoadShader(GL_VERTEX_SHADER, vertId, "shader.vert")) {
    return -1;
  }
  CheckErrors("load vert shader");

  if (false == LoadShader(GL_FRAGMENT_SHADER, fragId, "shader.frag")) {
    return -1;
  }
  CheckErrors("load frag shader");

  if (false == LinkShader(progId, vertId, fragId)) {
    return -1;
  }

  CheckErrors("link");

  {
    GLint vtxLoc = glGetAttribLocation(progId, "in_vertex");
    if (vtxLoc < 0) {
      printf("vertex loc not found.\n");
      exit(-1);
    }

    GLint tnLoc = glGetAttribLocation(progId, "in_normal");
    if (tnLoc < 0) {
      printf("normal loc not found.\n");
      exit(-1);
    }
  }

  glUseProgram(progId);
  CheckErrors("useProgram");

  SetupGLState(scene, progId);
  CheckErrors("SetupGLState");

  while(glfwWindowShouldClose(window) == GL_FALSE) {
    glfwPollEvents();
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    GLfloat mat[4][4];
    build_rotmatrix(mat, curr_quat);

    // camera(define it in projection matrix)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0], up[1], up[2]);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(&mat[0][0]);

    glScalef(scale, scale, scale);

    DrawScene(scene);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glFlush();

    glfwSwapBuffers(window);
  }

  glfwTerminate();
}
