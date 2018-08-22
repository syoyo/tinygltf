// ImGui BtGui binding with OpenGL based on Imgui Glfw binding.

#include "imgui_impl_btgui.h"

#ifdef _MSC_VER
#pragma warning(disable : 4100)
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

#include <imgui_internal.h>
#include <cstdio>

#define IMGUI_B3G_CONTROL (300)
#define IMGUI_B3G_SHIFT (301)
#define IMGUI_B3G_ALT (301)

// Data
static b3gDefaultOpenGLWindow* g_Window = NULL;
// static double       g_Time = 0.0f;
static bool g_MousePressed[3] = {false, false, false};
static float g_MouseWheel = 0.0f;
static GLuint g_FontTexture = 0;

// This is the main rendering function that you have to implement and provide to
// ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by
// (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_ImplBtGui_RenderDrawLists(ImDrawData* draw_data) {
  // Avoid rendering when minimized, scale coordinates for retina displays
  // (screen coordinates != framebuffer coordinates)
  ImGuiIO& io = ImGui::GetIO();
  int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
  int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
  if (fb_width == 0 || fb_height == 0) return;
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);

  // We are using the OpenGL fixed pipeline to make the example code simpler to
  // read!
  // Setup render state: alpha-blending enabled, no face culling, no depth
  // testing, scissor enabled, vertex/texcoord/color pointers.
  GLint last_texture;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  GLint last_viewport[4];
  glGetIntegerv(GL_VIEWPORT, last_viewport);
  glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnable(GL_TEXTURE_2D);
  // glUseProgram(0); // You may want this if using this code in an OpenGL 3+
  // context

  // Setup viewport, orthographic projection matrix
  glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

// Render command lists
#define OFFSETOF(TYPE, ELEMENT) ((size_t) & (((TYPE*)0)->ELEMENT))
  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    const unsigned char* vtx_buffer =
        (const unsigned char*)&cmd_list->VtxBuffer.front();
    const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();
    glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert),
                    (void*)(vtx_buffer + OFFSETOF(ImDrawVert, pos)));
    glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert),
                      (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert),
                   (void*)(vtx_buffer + OFFSETOF(ImDrawVert, col)));

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
        glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w),
                  (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                  (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
        glDrawElements(
            GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
            sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
            idx_buffer);
      }
      idx_buffer += pcmd->ElemCount;
    }
  }
#undef OFFSETOF

  // Restore modified state
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
  glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2],
             (GLsizei)last_viewport[3]);
}

static const char* ImGui_ImplBtGui_GetClipboardText(void* user_ctx) {
  // @todo
  return NULL;  // glfwGetClipboardString(g_Window);
}

static void ImGui_ImplBtGui_SetClipboardText(void* user_ctx, const char* text) {
  // @todo
  return;
  // glfwSetClipboardString(g_Window, text);
}

void ImGui_ImplBtGui_SetMouseButtonState(int button, bool pressed) {
  if (button >= 0 && button < 3) {
    g_MousePressed[button] = pressed;
  }
}

void ImGui_ImplBtGui_SetKeyState(int key, bool pressed) {
  ImGuiIO& io = ImGui::GetIO();

  // Remap keycode manually here.
  if (key == B3G_RETURN) {
    key = ImGuiKey_Enter;
  } else if ((key == B3G_BACKSPACE) || (key == 8)) {
    key = ImGuiKey_Backspace;
  } else if (key == 9) {
    key = ImGuiKey_Tab;
  } else if (key == B3G_DELETE) {
    key = ImGuiKey_Delete;
  } else if (key == B3G_END) {
    key = ImGuiKey_End;
  } else if (key == B3G_HOME) {
    key = ImGuiKey_Home;
  } else if (key == B3G_LEFT_ARROW) {
    key = ImGuiKey_LeftArrow;
  } else if (key == B3G_RIGHT_ARROW) {
    key = ImGuiKey_RightArrow;
  } else if (key == B3G_UP_ARROW) {
    key = ImGuiKey_UpArrow;
  } else if (key == B3G_DOWN_ARROW) {
    key = ImGuiKey_DownArrow;
  } else if (key == B3G_ESCAPE) {
    key = ImGuiKey_Escape;
  } else if (key == B3G_CONTROL) {
    key = IMGUI_B3G_CONTROL;
  } else if (key == B3G_ALT) {
    key = IMGUI_B3G_ALT;
  } else if (key == B3G_SHIFT) {
    key = IMGUI_B3G_SHIFT;
  }

  if (key >= 0 && key < 512) {
    io.KeysDown[key] = pressed;
  }

  io.KeyCtrl = io.KeysDown[IMGUI_B3G_CONTROL];
  io.KeyShift = io.KeysDown[IMGUI_B3G_SHIFT];
  io.KeyAlt = io.KeysDown[IMGUI_B3G_ALT];
  // io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] ||
  // io.KeysDown[GLFW_KEY_RIGHT_SUPER]; // @todo
}

void ImGui_ImplBtGui_SetChar(int c) {
  ImGuiIO& io = ImGui::GetIO();
  if (c > 0 && c < 0x10000) io.AddInputCharacter((unsigned short)c);
}

#if 0

void ImGui_ImplBtGui_ScrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset)
{
    g_MouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
}

#endif

bool ImGui_ImplBtGui_CreateDeviceObjects() {
  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

  // Upload texture to graphics system
  GLint last_texture;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGenTextures(1, &g_FontTexture);
  glBindTexture(GL_TEXTURE_2D, g_FontTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA,
               GL_UNSIGNED_BYTE, pixels);

  // Store our identifier
  io.Fonts->TexID = (void*)(intptr_t)g_FontTexture;

  // Restore state
  glBindTexture(GL_TEXTURE_2D, last_texture);

  return true;
}

void ImGui_ImplBtGui_InvalidateDeviceObjects() {
  if (g_FontTexture) {
    glDeleteTextures(1, &g_FontTexture);
    ImGui::GetIO().Fonts->TexID = 0;
    g_FontTexture = 0;
  }
}

bool ImGui_ImplBtGui_Init(b3gDefaultOpenGLWindow* window) {
  g_Window = window;

  ImGuiIO& io = ImGui::GetIO();

  io.KeyMap[ImGuiKey_Tab] = ImGuiKey_Tab;
  io.KeyMap[ImGuiKey_LeftArrow] = ImGuiKey_LeftArrow;
  io.KeyMap[ImGuiKey_RightArrow] = ImGuiKey_RightArrow;
  io.KeyMap[ImGuiKey_UpArrow] = ImGuiKey_UpArrow;
  io.KeyMap[ImGuiKey_DownArrow] = ImGuiKey_DownArrow;
  io.KeyMap[ImGuiKey_PageUp] = ImGuiKey_PageUp;
  io.KeyMap[ImGuiKey_PageDown] = ImGuiKey_PageDown;
  io.KeyMap[ImGuiKey_Home] = ImGuiKey_Home;
  io.KeyMap[ImGuiKey_End] = ImGuiKey_End;
  io.KeyMap[ImGuiKey_Delete] = ImGuiKey_Delete;
  io.KeyMap[ImGuiKey_Backspace] = ImGuiKey_Backspace;
  io.KeyMap[ImGuiKey_Enter] = ImGuiKey_Enter;
  io.KeyMap[ImGuiKey_Escape] = ImGuiKey_Escape;

  io.RenderDrawListsFn =
      ImGui_ImplBtGui_RenderDrawLists;  // Alternatively you can set this to
                                        // NULL and call ImGui::GetDrawData()
                                        // after ImGui::Render() to get the same
                                        // ImDrawData pointer.
  io.SetClipboardTextFn = ImGui_ImplBtGui_SetClipboardText;
  io.GetClipboardTextFn = ImGui_ImplBtGui_GetClipboardText;
#ifdef _WIN32
  // io.ImeWindowHandle = glfwGetWin32Window(g_Window);
#endif

#if 0
    if (install_callbacks)
    {
        glfwSetMouseButtonCallback(window, ImGui_ImplBtGui_MouseButtonCallback);
        glfwSetScrollCallback(window, ImGui_ImplBtGui_ScrollCallback);
        glfwSetKeyCallback(window, ImGui_ImplGlFw_KeyCallback);
        glfwSetCharCallback(window, ImGui_ImplBtGui_CharCallback);
    }
#endif

  return true;
}

void ImGui_ImplBtGui_Shutdown() {
  ImGui_ImplBtGui_InvalidateDeviceObjects();
  ImGui::Shutdown(ImGui::GetCurrentContext());
}

void ImGui_ImplBtGui_NewFrame(int mouse_x, int mouse_y) {
  if (!g_FontTexture) ImGui_ImplBtGui_CreateDeviceObjects();

  ImGuiIO& io = ImGui::GetIO();

  // Setup display size (every frame to accommodate for window resizing)
  int w, h;
  int display_w, display_h;
  assert(g_Window);
  // glfwGetWindowSize(g_Window, &w, &h);
  // glfwGetFramebufferSize(g_Window, &display_w, &display_h);
  w = g_Window->getWidth();
  h = g_Window->getHeight();
  display_w = g_Window->getWidth();  // @todo { support retina scale. }
  display_h = g_Window->getHeight();
  io.DisplaySize = ImVec2((float)w, (float)h);
  io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0,
                                      h > 0 ? ((float)display_h / h) : 0);

  // Setup time step
  // double current_time =  glfwGetTime();
  io.DeltaTime = (float)(1.0f / 60.0f);
  // g_Time = current_time;

  // Setup inputs
  // (we already got mouse wheel, keyboard keys & characters from glfw callbacks
  // polled in glfwPollEvents())
  // if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
  //{
  //    double mouse_x, mouse_y;
  //    glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
  //    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse
  //    position in screen coordinates (set to -1,-1 if no mouse / on another
  //    screen, etc.)
  //}
  // else
  {
    io.MousePos = ImVec2((float)mouse_x,
                         (float)mouse_y);  // Mouse position in screen
                                           // coordinates (set to -1,-1 if no
                                           // mouse / on another screen, etc.)
  }

  // @todo
  for (int i = 0; i < 3; i++) {
    io.MouseDown[i] = g_MousePressed[i];
    // g_MousePressed[i] = false;
  }

  io.MouseWheel = g_MouseWheel;
  g_MouseWheel = 0.0f;

  // Hide OS mouse cursor if ImGui is drawing it
  // glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ?
  // GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

  // Start the frame
  ImGui::NewFrame();
}
