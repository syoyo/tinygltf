#define GLFW_EXPOSE_NATIVE_WIN32
#define TINYGLTF_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <spdlog/spdlog.h>
#include <tiny_gltf.h>

#undef GLFW_EXPOSE_NATIVE_WIN32
#undef TINYGLTF_IMPLEMENTATION
#undef STBI_MSC_SECURE_CRT
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_IMAGE_WRITE_IMPLEMENTATION

#include <iostream>
#include <string>

#include "Viewer.h"

static void onError(int error, const char* message) {
  spdlog::error("[{}] {}", error, message);
}

static void onRender(Viewer* pViewer, double deltaTime) {
  pViewer->update(deltaTime);
  pViewer->render(deltaTime);
}

int main(int argc, char* argv[]) {
  tinygltf::TinyGLTF context;

  tinygltf::Model model;
  std::string error;
  std::string warning;
  if (!context.LoadASCIIFromFile(&model, &error, &warning, argv[1])) {
    if (!error.empty()) {
      spdlog::error("{}", error);
    }

    if (!warning.empty()) {
      spdlog::warn("{}", warning);
    }

    exit(EXIT_FAILURE);
  }

  GLFWwindow* pWindow;

  glfwSetErrorCallback(onError);

  if (!glfwInit()) {
    spdlog::error("Fail to initialize GLFW!!!");
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  pWindow = glfwCreateWindow(512, 512, "dxview", nullptr, nullptr);
  if (!pWindow) {
    spdlog::error("Fail to create GLFWwindow!!!");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  auto pViewer = std::make_unique<Viewer>(glfwGetWin32Window(pWindow), &model);
  if (!pViewer) {
    spdlog::error("Fail to create Viewer");
    glfwDestroyWindow(pWindow);
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  auto prevTimeStamp = glfwGetTime();
  while (!glfwWindowShouldClose(pWindow)) {
    auto currTimeStamp = glfwGetTime();

    onRender(pViewer.get(), currTimeStamp - prevTimeStamp);
    prevTimeStamp = currTimeStamp;

    glfwPollEvents();
  }

  glfwDestroyWindow(pWindow);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}