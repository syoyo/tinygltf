// ImGui BtGui binding with OpenGL

#include <imgui.h>

class b3gDefaultOpenGLWindow;

IMGUI_API bool ImGui_ImplBtGui_Init(b3gDefaultOpenGLWindow* window);
IMGUI_API void ImGui_ImplBtGui_Shutdown();
IMGUI_API void ImGui_ImplBtGui_NewFrame(int mouse_x, int mouse_y);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void ImGui_ImplBtGui_InvalidateDeviceObjects();
IMGUI_API bool ImGui_ImplBtGui_CreateDeviceObjects();

IMGUI_API void ImGui_ImplBtGui_SetKeyState(int key, bool pressed);
IMGUI_API void ImGui_ImplBtGui_SetChar(int c);
IMGUI_API void ImGui_ImplBtGui_SetMouseButtonState(int button, bool pressed);
