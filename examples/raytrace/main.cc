/*
The MIT License (MIT)

Copyright (c) 2015 - 2016 Light Transport Entertainment, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifdef _MSC_VER
#pragma warning(disable : 4244)
#endif

#define USE_OPENGL2
#include "OpenGLWindow/OpenGLInclude.h"
#ifdef _WIN32
#include "OpenGLWindow/Win32OpenGLWindow.h"
#elif defined __APPLE__
#include "OpenGLWindow/MacOpenGLWindow.h"
#else
// assume linux
#include "OpenGLWindow/X11OpenGLWindow.h"
#endif

#ifdef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <atomic>  // C++11
#include <chrono>  // C++11
#include <mutex>   // C++11
#include <thread>  // C++11

#include "imgui.h"
#include "imgui_impl_btgui.h"

#include "ImGuizmo.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4201)
#endif

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/mat4x4.hpp"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "gltf-loader.h"
#include "nanosg.h"
#include "obj-loader.h"
#include "render-config.h"
#include "render.h"
#include "trackball.h"

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

b3gDefaultOpenGLWindow *window = 0;
int gWidth = 512;
int gHeight = 512;
int gMousePosX = -1, gMousePosY = -1;
bool gMouseLeftDown = false;

// FIX issue when max passes is done - no modes is switched. pass must be set to
// 0 when mode is changed
int gShowBufferMode_prv = SHOW_BUFFER_COLOR;
int gShowBufferMode = SHOW_BUFFER_COLOR;

bool gTabPressed = false;
bool gShiftPressed = false;
float gShowPositionScale = 1.0f;
float gShowDepthRange[2] = {10.0f, 20.f};
bool gShowDepthPeseudoColor = true;
float gCurrQuat[4] = {0.0f, 0.0f, 0.0f, 0.0f};
float gPrevQuat[4] = {0.0f, 0.0f, 0.0f, 0.0f};

static nanosg::Scene<float, example::Mesh<float> > gScene;
static example::Asset gAsset;
static std::vector<nanosg::Node<float, example::Mesh<float> > > gNodes;

std::atomic<bool> gRenderQuit;
std::atomic<bool> gRenderRefresh;
std::atomic<bool> gRenderCancel;
std::atomic<bool> gSceneDirty;
example::RenderConfig gRenderConfig;
std::mutex gMutex;

struct RenderLayer {
  std::vector<float> displayRGBA;  // Accumurated image.
  std::vector<float> rgba;
  std::vector<float> auxRGBA;        // Auxiliary buffer
  std::vector<int> sampleCounts;     // Sample num counter for each pixel.
  std::vector<float> normalRGBA;     // For visualizing normal
  std::vector<float> positionRGBA;   // For visualizing position
  std::vector<float> depthRGBA;      // For visualizing depth
  std::vector<float> texCoordRGBA;   // For visualizing texcoord
  std::vector<float> varyCoordRGBA;  // For visualizing varycentric coord
};

RenderLayer gRenderLayer;

struct Camera {
  glm::mat4 view;
  glm::mat4 projection;
};

struct ManipConfig {
  glm::vec3 snapTranslation;
  glm::vec3 snapRotation;
  glm::vec3 snapScale;
};

void RequestRender() {
  {
    std::lock_guard<std::mutex> guard(gMutex);
    gRenderConfig.pass = 0;
  }

  gRenderRefresh = true;
  gRenderCancel = true;
}

void RenderThread() {
  {
    std::lock_guard<std::mutex> guard(gMutex);
    gRenderConfig.pass = 0;
  }

  while (1) {
    if (gRenderQuit) return;

    if (!gRenderRefresh || gRenderConfig.pass >= gRenderConfig.max_passes) {
      // Give some cycles to this thread.
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    auto startT = std::chrono::system_clock::now();

    // Initialize display buffer for the first pass.
    bool initial_pass = false;
    {
      std::lock_guard<std::mutex> guard(gMutex);
      if (gRenderConfig.pass == 0) {
        initial_pass = true;
      }
    }

    if (gSceneDirty) {
      gScene.Commit();
      gSceneDirty = false;
    }

    gRenderCancel = false;
    // gRenderCancel may be set to true in main loop.
    // Render() will repeatedly check this flag inside the rendering loop.

    bool ret = example::Renderer::Render(
        &gRenderLayer.rgba.at(0), &gRenderLayer.auxRGBA.at(0),
        &gRenderLayer.sampleCounts.at(0), gCurrQuat, gScene, gAsset,
        gRenderConfig, gRenderCancel,
        gShowBufferMode  // added mode passing
    );

    if (ret) {
      std::lock_guard<std::mutex> guard(gMutex);

      gRenderConfig.pass++;
    }

    auto endT = std::chrono::system_clock::now();

    std::chrono::duration<double, std::milli> ms = endT - startT;

    // std::cout << ms.count() << " [ms]\n";
  }
}

void InitRender(example::RenderConfig *rc) {
  rc->pass = 0;

  rc->max_passes = 128;

  gRenderLayer.sampleCounts.resize(rc->width * rc->height);
  std::fill(gRenderLayer.sampleCounts.begin(), gRenderLayer.sampleCounts.end(),
            0.0);

  gRenderLayer.displayRGBA.resize(rc->width * rc->height * 4);
  std::fill(gRenderLayer.displayRGBA.begin(), gRenderLayer.displayRGBA.end(),
            0.0);

  gRenderLayer.rgba.resize(rc->width * rc->height * 4);
  std::fill(gRenderLayer.rgba.begin(), gRenderLayer.rgba.end(), 0.0);

  gRenderLayer.auxRGBA.resize(rc->width * rc->height * 4);
  std::fill(gRenderLayer.auxRGBA.begin(), gRenderLayer.auxRGBA.end(), 0.0);

  gRenderLayer.normalRGBA.resize(rc->width * rc->height * 4);
  std::fill(gRenderLayer.normalRGBA.begin(), gRenderLayer.normalRGBA.end(),
            0.0);

  gRenderLayer.positionRGBA.resize(rc->width * rc->height * 4);
  std::fill(gRenderLayer.positionRGBA.begin(), gRenderLayer.positionRGBA.end(),
            0.0);

  gRenderLayer.depthRGBA.resize(rc->width * rc->height * 4);
  std::fill(gRenderLayer.depthRGBA.begin(), gRenderLayer.depthRGBA.end(), 0.0);

  gRenderLayer.texCoordRGBA.resize(rc->width * rc->height * 4);
  std::fill(gRenderLayer.texCoordRGBA.begin(), gRenderLayer.texCoordRGBA.end(),
            0.0);

  gRenderLayer.varyCoordRGBA.resize(rc->width * rc->height * 4);
  std::fill(gRenderLayer.varyCoordRGBA.begin(),
            gRenderLayer.varyCoordRGBA.end(), 0.0);

  rc->normalImage = &gRenderLayer.normalRGBA.at(0);
  rc->positionImage = &gRenderLayer.positionRGBA.at(0);
  rc->depthImage = &gRenderLayer.depthRGBA.at(0);
  rc->texcoordImage = &gRenderLayer.texCoordRGBA.at(0);
  rc->varycoordImage = &gRenderLayer.varyCoordRGBA.at(0);

  trackball(gCurrQuat, 0.0f, 0.0f, 0.0f, 0.0f);
}

void checkErrors(std::string desc) {
  GLenum e = glGetError();
  if (e != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error in \"%s\": %d (%d)\n", desc.c_str(), e, e);
    exit(20);
  }
}

static int CreateDisplayTextureGL(const float *data, int width, int height,
                                  int components) {
  GLuint id;
  glGenTextures(1, &id);

  GLint last_texture;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  GLenum format = GL_RGBA;
  if (components == 1) {
    format = GL_LUMINANCE;
  } else if (components == 2) {
    format = GL_LUMINANCE_ALPHA;
  } else if (components == 3) {
    format = GL_RGB;
  } else if (components == 4) {
    format = GL_RGBA;
  } else {
    assert(0);  // "Invalid components"
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_FLOAT,
               data);

  glBindTexture(GL_TEXTURE_2D, last_texture);

  return static_cast<int>(id);
}

void keyboardCallback(int keycode, int state) {
  printf("hello key %d, state %d(ctrl %d)\n", keycode, state,
         window->isModifierKeyPressed(B3G_CONTROL));
  // if (keycode == 'q' && window && window->isModifierKeyPressed(B3G_SHIFT)) {
  if (keycode == 27) {
    if (window) window->setRequestExit();
  } else if (keycode == ' ') {
    // reset.
    trackball(gCurrQuat, 0.0f, 0.0f, 0.0f, 0.0f);
    // clear buffer.
    memset(gRenderLayer.rgba.data(), 0,
           sizeof(float) * gRenderConfig.width * gRenderConfig.height * 4);
    memset(gRenderLayer.sampleCounts.data(), 0,
           sizeof(int) * gRenderConfig.width * gRenderConfig.height);
  } else if (keycode == 9) {
    gTabPressed = (state == 1);
  } else if (keycode == B3G_SHIFT) {
    gShiftPressed = (state == 1);
  }

  ImGui_ImplBtGui_SetKeyState(keycode, (state == 1));

  if (keycode >= 32 && keycode <= 126) {
    if (state == 1) {
      ImGui_ImplBtGui_SetChar(keycode);
    }
  }
}

void mouseMoveCallback(float x, float y) {
  if (gMouseLeftDown) {
    if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
      gSceneDirty = true;
      // RequestRender();
    } else {
      float w = static_cast<float>(gRenderConfig.width);
      float h = static_cast<float>(gRenderConfig.height);

      float y_offset = gHeight - h;

      if (gTabPressed) {
        const float dolly_scale = 0.1f;
        gRenderConfig.eye[2] += dolly_scale * (gMousePosY - y);
        gRenderConfig.look_at[2] += dolly_scale * (gMousePosY - y);
      } else if (gShiftPressed) {
        const float trans_scale = 0.02f;
        gRenderConfig.eye[0] += trans_scale * (gMousePosX - x);
        gRenderConfig.eye[1] -= trans_scale * (gMousePosY - y);
        gRenderConfig.look_at[0] += trans_scale * (gMousePosX - x);
        gRenderConfig.look_at[1] -= trans_scale * (gMousePosY - y);

      } else {
        // Adjust y.
        trackball(gPrevQuat, (2.f * gMousePosX - w) / (float)w,
                  (h - 2.f * (gMousePosY - y_offset)) / (float)h,
                  (2.f * x - w) / (float)w,
                  (h - 2.f * (y - y_offset)) / (float)h);
        add_quats(gPrevQuat, gCurrQuat, gCurrQuat);
      }
      RequestRender();
    }
  }

  gMousePosX = (int)x;
  gMousePosY = (int)y;
}

void mouseButtonCallback(int button, int state, float x, float y) {
  (void)x;
  (void)y;
  ImGui_ImplBtGui_SetMouseButtonState(button, (state == 1));

  if (button == 0 && !state)
    gMouseLeftDown = false;  // prevent sticky trackball after using gizmo

  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
    if (button == 0 && !state) {
      if (ImGuizmo::IsUsing()) {
        gSceneDirty = true;
        RequestRender();
      }
    }
  } else {
    // left button
    if (button == 0) {
      if (state) {
        gMouseLeftDown = true;
        if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
        } else {
          trackball(gPrevQuat, 0.0f, 0.0f, 0.0f, 0.0f);
        }
      } else {
      }
    }
  }
}

void resizeCallback(float width, float height) {
  // GLfloat h = (GLfloat)height / (GLfloat)width;
  GLfloat xmax, znear, zfar;

  znear = 1.0f;
  zfar = 1000.0f;
  xmax = znear * 0.5f;

  gWidth = static_cast<int>(width);
  gHeight = static_cast<int>(height);
}

inline float pseudoColor(float v, int ch) {
  if (ch == 0) {  // red
    if (v <= 0.5f)
      return 0.f;
    else if (v < 0.75f)
      return (v - 0.5f) / 0.25f;
    else
      return 1.f;
  } else if (ch == 1) {  // green
    if (v <= 0.25f)
      return v / 0.25f;
    else if (v < 0.75f)
      return 1.f;
    else
      return 1.f - (v - 0.75f) / 0.25f;
  } else if (ch == 2) {  // blue
    if (v <= 0.25f)
      return 1.f;
    else if (v < 0.5f)
      return 1.f - (v - 0.25f) / 0.25f;
    else
      return 0.f;
  } else {  // alpha
    return 1.f;
  }
}

void UpdateDisplayTextureGL(GLint tex_id, int width, int height) {
  if (tex_id < 0) {
    // ???
    return;
  }

  std::vector<float> buf;
  buf.resize(width * height * 4);

  if (gShowBufferMode == SHOW_BUFFER_COLOR) {
    // normalize
    for (size_t i = 0; i < buf.size() / 4; i++) {
      buf[4 * i + 0] = gRenderLayer.rgba[4 * i + 0];
      buf[4 * i + 1] = gRenderLayer.rgba[4 * i + 1];
      buf[4 * i + 2] = gRenderLayer.rgba[4 * i + 2];
      buf[4 * i + 3] = gRenderLayer.rgba[4 * i + 3];
      if (gRenderLayer.sampleCounts[i] > 0) {
        buf[4 * i + 0] /= static_cast<float>(gRenderLayer.sampleCounts[i]);
        buf[4 * i + 1] /= static_cast<float>(gRenderLayer.sampleCounts[i]);
        buf[4 * i + 2] /= static_cast<float>(gRenderLayer.sampleCounts[i]);
        buf[4 * i + 3] /= static_cast<float>(gRenderLayer.sampleCounts[i]);
      }
    }
  } else if (gShowBufferMode == SHOW_BUFFER_NORMAL) {
    for (size_t i = 0; i < buf.size(); i++) {
      buf[i] = gRenderLayer.normalRGBA[i];
    }
  } else if (gShowBufferMode == SHOW_BUFFER_POSITION) {
    for (size_t i = 0; i < buf.size(); i++) {
      buf[i] = gRenderLayer.positionRGBA[i] * gShowPositionScale;
    }
  } else if (gShowBufferMode == SHOW_BUFFER_DEPTH) {
    float d_min = std::min(gShowDepthRange[0], gShowDepthRange[1]);
    float d_diff = fabsf(gShowDepthRange[1] - gShowDepthRange[0]);
    d_diff = std::max(d_diff, std::numeric_limits<float>::epsilon());
    for (size_t i = 0; i < buf.size(); i++) {
      float v = (gRenderLayer.depthRGBA[i] - d_min) / d_diff;
      if (gShowDepthPeseudoColor) {
        buf[i] = pseudoColor(v, i % 4);
      } else {
        buf[i] = v;
      }
    }
  } else if (gShowBufferMode == SHOW_BUFFER_TEXCOORD) {
    for (size_t i = 0; i < buf.size(); i++) {
      buf[i] = gRenderLayer.texCoordRGBA[i];
    }
  } else if (gShowBufferMode == SHOW_BUFFER_VARYCOORD) {
    for (size_t i = 0; i < buf.size(); i++) {
      buf[i] = gRenderLayer.varyCoordRGBA[i];
    }
  }

  // Flip Y
  std::vector<float> disp;
  disp.resize(width * height * 4);
  {
    for (size_t y = 0; y < height; y++) {
      memcpy(&disp[4 * (y * width)], &buf[4 * ((height - y - 1) * width)],
             sizeof(float) * 4 * width);
    }
  }

  glBindTexture(GL_TEXTURE_2D, tex_id);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT,
                  disp.data());
  glBindTexture(GL_TEXTURE_2D, 0);

  // glRasterPos2i(-1, -1);
  // glDrawPixels(width, height, GL_RGBA, GL_FLOAT,
  //             static_cast<const GLvoid*>(&buf.at(0)));
}

void EditTransform(const ManipConfig &config, const Camera &camera,
                   glm::mat4 &matrix) {
  static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
  static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
  if (ImGui::IsKeyPressed(90)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
  if (ImGui::IsKeyPressed(69)) mCurrentGizmoOperation = ImGuizmo::ROTATE;
  if (ImGui::IsKeyPressed(82))  // r Key
    mCurrentGizmoOperation = ImGuizmo::SCALE;
  if (ImGui::RadioButton("Translate",
                         mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
    mCurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
    mCurrentGizmoOperation = ImGuizmo::SCALE;
  float matrixTranslation[3], matrixRotation[3], matrixScale[3];
  ImGuizmo::DecomposeMatrixToComponents(&matrix[0][0], matrixTranslation,
                                        matrixRotation, matrixScale);
  ImGui::InputFloat3("Tr", matrixTranslation, 3);
  ImGui::InputFloat3("Rt", matrixRotation, 3);
  ImGui::InputFloat3("Sc", matrixScale, 3);
  ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                          matrixScale, &matrix[0][0]);

  if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
    if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
      mCurrentGizmoMode = ImGuizmo::LOCAL;
    ImGui::SameLine();
    if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
      mCurrentGizmoMode = ImGuizmo::WORLD;
  }
  static bool useSnap(false);
  if (ImGui::IsKeyPressed(83)) useSnap = !useSnap;
  ImGui::Checkbox("", &useSnap);
  ImGui::SameLine();
  glm::vec3 snap;
  switch (mCurrentGizmoOperation) {
    case ImGuizmo::TRANSLATE:
      snap = config.snapTranslation;
      ImGui::InputFloat3("Snap", &snap.x);
      break;
    case ImGuizmo::ROTATE:
      snap = config.snapRotation;
      ImGui::InputFloat("Angle Snap", &snap.x);
      break;
    case ImGuizmo::SCALE:
      snap = config.snapScale;
      ImGui::InputFloat("Scale Snap", &snap.x);
      break;
  }
  ImGuiIO &io = ImGui::GetIO();
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
  ImGuizmo::Manipulate(&camera.view[0][0], &camera.projection[0][0],
                       mCurrentGizmoOperation, mCurrentGizmoMode, &matrix[0][0],
                       NULL, useSnap ? &snap.x : NULL);
}

void DrawMesh(const example::Mesh<float> *mesh) {
  // TODO(LTE): Use vertex array or use display list.

  glBegin(GL_TRIANGLES);

  if (!mesh->facevarying_normals.empty()) {
    for (size_t i = 0; i < mesh->faces.size() / 3; i++) {
      unsigned int f0 = mesh->faces[3 * i + 0];
      unsigned int f1 = mesh->faces[3 * i + 1];
      unsigned int f2 = mesh->faces[3 * i + 2];

      glNormal3f(mesh->facevarying_normals[9 * i + 0],
                 mesh->facevarying_normals[9 * i + 1],
                 mesh->facevarying_normals[9 * i + 2]);
      glVertex3f(mesh->vertices[3 * f0 + 0], mesh->vertices[3 * f0 + 1],
                 mesh->vertices[3 * f0 + 2]);
      glNormal3f(mesh->facevarying_normals[9 * i + 3],
                 mesh->facevarying_normals[9 * i + 4],
                 mesh->facevarying_normals[9 * i + 5]);
      glVertex3f(mesh->vertices[3 * f1 + 0], mesh->vertices[3 * f1 + 1],
                 mesh->vertices[3 * f1 + 2]);
      glNormal3f(mesh->facevarying_normals[9 * i + 6],
                 mesh->facevarying_normals[9 * i + 7],
                 mesh->facevarying_normals[9 * i + 8]);
      glVertex3f(mesh->vertices[3 * f2 + 0], mesh->vertices[3 * f2 + 1],
                 mesh->vertices[3 * f2 + 2]);
    }

  } else {
    for (size_t i = 0; i < mesh->faces.size() / 3; i++) {
      unsigned int f0 = mesh->faces[3 * i + 0];
      unsigned int f1 = mesh->faces[3 * i + 1];
      unsigned int f2 = mesh->faces[3 * i + 2];

      glVertex3f(mesh->vertices[3 * f0 + 0], mesh->vertices[3 * f0 + 1],
                 mesh->vertices[3 * f0 + 2]);
      glVertex3f(mesh->vertices[3 * f1 + 0], mesh->vertices[3 * f1 + 1],
                 mesh->vertices[3 * f1 + 2]);
      glVertex3f(mesh->vertices[3 * f2 + 0], mesh->vertices[3 * f2 + 1],
                 mesh->vertices[3 * f2 + 2]);
    }
  }

  glEnd();
}

void DrawNode(const nanosg::Node<float, example::Mesh<float> > &node) {
  glPushMatrix();
  glMultMatrixf(node.GetLocalXformPtr());

  if (node.GetMesh()) {
    DrawMesh(node.GetMesh());
  }

  for (size_t i = 0; i < node.GetChildren().size(); i++) {
    DrawNode(node.GetChildren()[i]);
  }

  glPopMatrix();
}

// Draw scene with OpenGL
void DrawScene(const nanosg::Scene<float, example::Mesh<float> > &scene,
               const Camera &camera) {
  glEnable(GL_DEPTH_TEST);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);

  // FIXME(LTE): Use scene bounding box.
  const float light0_pos[4] = {1000.0f, 1000.0f, 1000.0f, 0.0f};
  const float light1_pos[4] = {-1000.0f, -1000.0f, -1000.0f, 0.0f};

  const float light_diffuse[4] = {0.5f, 0.5f, 0.5f, 1.0f};

  glLightfv(GL_LIGHT0, GL_POSITION, &light0_pos[0]);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, &light_diffuse[0]);
  glLightfv(GL_LIGHT1, GL_POSITION, &light1_pos[0]);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, &light_diffuse[0]);

  const std::vector<nanosg::Node<float, example::Mesh<float> > > &root_nodes =
      scene.GetNodes();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadMatrixf(&camera.projection[0][0]);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadMatrixf(&camera.view[0][0]);

  for (size_t i = 0; i < root_nodes.size(); i++) {
    DrawNode(root_nodes[i]);
  }

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHT1);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
}

void BuildSceneItems(std::vector<std::string> *display_names,
                     std::vector<std::string> *names,
                     const nanosg::Node<float, example::Mesh<float> > &node,
                     int indent) {
  if (node.GetName().empty()) {
    // Skip a node with empty name.
    return;
  }

  std::stringstream ss;
  for (int i = 0; i < indent; i++) {
    ss << " ";
  }

  ss << node.GetName();
  std::string display_name = ss.str();

  display_names->push_back(display_name);
  names->push_back(node.GetName());

  for (size_t i = 0; i < node.GetChildren().size(); i++) {
    BuildSceneItems(display_names, names, node.GetChildren()[i], indent + 1);
  }
}

// tigra: add default material
example::Material default_material;

int main(int argc, char **argv) {
  std::string config_filename = "config.json";

  if (argc > 1) {
    config_filename = argv[1];
  }

  // load config
  {
    bool ret =
        example::LoadRenderConfig(&gRenderConfig, config_filename.c_str());
    if (!ret) {
      std::cerr << "Failed to load [ " << config_filename << " ]" << std::endl;
      return -1;
    }
  }

  // construct the scene
  {
    std::vector<example::Mesh<float> > meshes;
    std::vector<example::Material> materials;
    std::vector<example::Texture> textures;

    // tigra: set default material to 95% white diffuse
    default_material.diffuse[0] = 0.95f;
    default_material.diffuse[1] = 0.95f;
    default_material.diffuse[2] = 0.95f;

    default_material.specular[0] = 0;
    default_material.specular[1] = 0;
    default_material.specular[2] = 0;

    // Material pushed as first material on the list
    materials.push_back(default_material);

    if (!gRenderConfig.obj_filename.empty()) {
      bool ret = LoadObj(gRenderConfig.obj_filename, gRenderConfig.scene_scale,
                         &meshes, &materials, &textures);
      if (!ret) {
        std::cerr << "Failed to load .obj [ " << gRenderConfig.obj_filename
                  << " ]" << std::endl;
        return -1;
      }
    }

    if (!gRenderConfig.gltf_filename.empty()) {
      std::cout << "Found gltf file : " << gRenderConfig.gltf_filename << "\n";

      bool ret =
          LoadGLTF(gRenderConfig.gltf_filename, gRenderConfig.scene_scale,
                   &meshes, &materials, &textures);
      if (!ret) {
        std::cerr << "Failed to load glTF file [ "
                  << gRenderConfig.gltf_filename << " ]" << std::endl;
        return -1;
      }
    }

    if (textures.size() > 0) {
      materials[0].diffuse_texid = 0;
    }

    gAsset.materials = materials;
    gAsset.default_material = default_material;
    gAsset.textures = textures;

    for (size_t n = 0; n < meshes.size(); n++) {
      size_t mesh_id = gAsset.meshes.size();
      gAsset.meshes.push_back(meshes[mesh_id]);
    }

    for (size_t n = 0; n < gAsset.meshes.size(); n++) {
      nanosg::Node<float, example::Mesh<float> > node(&gAsset.meshes[n]);

      // case where the name of a mesh isn't defined in the loaded file
      if (gAsset.meshes[n].name.empty()) {
        std::string generatedName = "unnamed_" + std::to_string(n);
        gAsset.meshes[n].name = generatedName;
        meshes[n].name = generatedName;
      }

      node.SetName(meshes[n].name);
      node.SetLocalXform(meshes[n].pivot_xform);  // Use mesh's pivot transform
                                                  // as node's local transform.
      gNodes.push_back(node);

      gScene.AddNode(node);
    }

    if (!gScene.Commit()) {
      std::cerr << "Failed to commit the scene." << std::endl;
      return -1;
    }

    float bmin[3], bmax[3];
    gScene.GetBoundingBox(bmin, bmax);
    printf("  # of nodes               : %d\n", int(gNodes.size()));
    printf("  Scene Bmin               : %f, %f, %f\n", bmin[0], bmin[1],
           bmin[2]);
    printf("  Scene Bmax               : %f, %f, %f\n", bmax[0], bmax[1],
           bmax[2]);
  }

  std::vector<const char *> imgui_node_names;
  std::vector<std::string> display_node_names;
  std::vector<std::string> node_names;
  std::map<int, nanosg::Node<float, example::Mesh<float> > *> node_map;

  {
    for (size_t i = 0; i < gScene.GetNodes().size(); i++) {
      BuildSceneItems(&display_node_names, &node_names, gScene.GetNodes()[i],
                      /* indent */ 0);
    }

    // List of strings for imgui.
    // Assume nodes in the scene does not change.
    for (size_t i = 0; i < display_node_names.size(); i++) {
      // std::cout << "name : " << display_node_names[i] << std::endl;
      imgui_node_names.push_back(display_node_names[i].c_str());
    }

    // Construct list index <-> Node ptr map.
    for (size_t i = 0; i < node_names.size(); i++) {
      nanosg::Node<float, example::Mesh<float> > *node;

      if (gScene.FindNode(node_names[i], &node)) {
        // std::cout << "id : " << i << ", name : " << node_names[i] <<
        // std::endl;
        node_map[i] = node;
      }
    }
  }

  window = new b3gDefaultOpenGLWindow;
  b3gWindowConstructionInfo ci;
#ifdef USE_OPENGL2
  ci.m_openglVersion = 2;
#endif
  ci.m_width = 1024;
  ci.m_height = 800;
  window->createWindow(ci);

  window->setWindowTitle("view");

#ifndef __APPLE__
#ifndef _WIN32
  // some Linux implementations need the 'glewExperimental' to be true
  glewExperimental = GL_TRUE;
#endif
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    exit(-1);
  }

  if (!GLEW_VERSION_2_1) {
    fprintf(stderr, "OpenGL 2.1 is not available\n");
    exit(-1);
  }
#endif

  InitRender(&gRenderConfig);

  checkErrors("init");

  window->setMouseButtonCallback(mouseButtonCallback);
  window->setMouseMoveCallback(mouseMoveCallback);
  checkErrors("mouse");
  window->setKeyboardCallback(keyboardCallback);
  checkErrors("keyboard");
  window->setResizeCallback(resizeCallback);
  checkErrors("resize");

  ImGui::CreateContext();
  ImGui_ImplBtGui_Init(window);

  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("./DroidSans.ttf", 15.0f);

  glm::mat4 projection(1.0f);
  glm::mat4 view(1.0f);
  glm::mat4 matrix(1.0f);

  Camera camera;

  std::thread renderThread(RenderThread);

  // Trigger initial rendering request
  RequestRender();

  while (!window->requestedExit()) {
    window->startRendering();

    checkErrors("begin frame");

    ImGui_ImplBtGui_NewFrame(gMousePosX, gMousePosY);

    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);

    // ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    ImGui::Begin("UI");
    {
      static float col[3] = {0, 0, 0};
      static float f = 0.0f;
      // if (ImGui::ColorEdit3("color", col)) {
      //  RequestRender();
      //}
      // ImGui::InputFloat("intensity", &f);
      if (ImGui::InputFloat3("eye", gRenderConfig.eye)) {
        RequestRender();
      }
      if (ImGui::InputFloat3("up", gRenderConfig.up)) {
        RequestRender();
      }
      if (ImGui::InputFloat3("look_at", gRenderConfig.look_at)) {
        RequestRender();
      }

      ImGui::RadioButton("color", &gShowBufferMode, SHOW_BUFFER_COLOR);
      ImGui::SameLine();
      ImGui::RadioButton("normal", &gShowBufferMode, SHOW_BUFFER_NORMAL);
      ImGui::SameLine();
      ImGui::RadioButton("position", &gShowBufferMode, SHOW_BUFFER_POSITION);
      ImGui::SameLine();
      ImGui::RadioButton("depth", &gShowBufferMode, SHOW_BUFFER_DEPTH);
      ImGui::SameLine();
      ImGui::RadioButton("texcoord", &gShowBufferMode, SHOW_BUFFER_TEXCOORD);
      ImGui::SameLine();
      ImGui::RadioButton("varycoord", &gShowBufferMode, SHOW_BUFFER_VARYCOORD);

      ImGui::InputFloat("show pos scale", &gShowPositionScale);

      ImGui::InputFloat2("show depth range", gShowDepthRange);
      ImGui::Checkbox("show depth pseudo color", &gShowDepthPeseudoColor);
    }

    ImGui::End();

    glViewport(0, 0, window->getWidth(), window->getHeight());
    glClearColor(0.0f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    checkErrors("clear");

    // fix max passes issue
    if (gShowBufferMode_prv != gShowBufferMode) {
      gRenderConfig.pass = 0;
      gShowBufferMode_prv = gShowBufferMode;
    }

    // Render display window
    {
      static GLint gl_texid = -1;
      if (gl_texid < 0) {
        gl_texid = CreateDisplayTextureGL(NULL, gRenderConfig.width,
                                          gRenderConfig.height, 4);
      }

      // Refresh texture until rendering finishes.
      if (gRenderConfig.pass < gRenderConfig.max_passes) {
        // FIXME(LTE): Do not update GL texture frequently.
        UpdateDisplayTextureGL(gl_texid, gRenderConfig.width,
                               gRenderConfig.height);
      }

      ImGui::Begin("Render");
      ImTextureID tex_id =
          reinterpret_cast<void *>(static_cast<intptr_t>(gl_texid));
      ImGui::Image(tex_id, ImVec2(256, 256), ImVec2(0, 0),
                   ImVec2(1, 1));  // Setup camera and draw imguizomo

      ImGui::End();
    }

    // scene graph tree
    glm::mat4 node_matrix;
    static int node_selected_index = 0;
    {
      ImGui::Begin("Scene");

      ImGui::ListBox("Node list", &node_selected_index, imgui_node_names.data(),
                     imgui_node_names.size(), 16);

      auto node_selected = node_map.at(node_selected_index);
      node_matrix = glm::make_mat4(node_selected->GetLocalXformPtr());

      ImGui::End();
    }

    {
      ImGui::Begin("Transform");

      static ImGuizmo::OPERATION guizmo_op(ImGuizmo::ROTATE);
      static ImGuizmo::MODE guizmo_mode(ImGuizmo::WORLD);

      glm::vec3 eye, lookat, up;
      eye[0] = gRenderConfig.eye[0];
      eye[1] = gRenderConfig.eye[1];
      eye[2] = gRenderConfig.eye[2];

      lookat[0] = gRenderConfig.look_at[0];
      lookat[1] = gRenderConfig.look_at[1];
      lookat[2] = gRenderConfig.look_at[2];

      up[0] = gRenderConfig.up[0];
      up[1] = gRenderConfig.up[1];
      up[2] = gRenderConfig.up[2];

      // NOTE(LTE): w, then (x,y,z) for glm::quat.
      glm::quat rot =
          glm::quat(gCurrQuat[3], gCurrQuat[0], gCurrQuat[1], gCurrQuat[2]);
      glm::mat4 rm = glm::mat4_cast(rot);

      view = glm::lookAt(eye, lookat, up) * glm::inverse(glm::mat4_cast(rot));
      projection = glm::perspective(
          45.0f, float(window->getWidth()) / float(window->getHeight()), 0.01f,
          1000.0f);

      camera.view = view;
      camera.projection = projection;
      ManipConfig manip_config;

      EditTransform(manip_config, camera, node_matrix);

      float mat[4][4];
      memcpy(mat, &node_matrix[0][0], sizeof(float) * 16);
      node_map[node_selected_index]->SetLocalXform(mat);

      checkErrors("edit_transform");

      ImGui::End();
    }

    // Draw scene in OpenGL
    DrawScene(gScene, camera);

    // Draw imgui
    ImGui::Render();

    checkErrors("im render");

    window->endRendering();

    // Give some cycles to this thread.
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  {
    gRenderCancel = true;
    gRenderQuit = true;
    renderThread.join();
  }

  ImGui_ImplBtGui_Shutdown();
  delete window;

  return EXIT_SUCCESS;
}
