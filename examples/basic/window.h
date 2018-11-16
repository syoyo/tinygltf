#pragma once

#include <GLFW/glfw3.h>

class Window
{
public:
	GLFWwindow* window;

	Window(int x, int y, const char* title);
	~Window();
	void Resize();
	int Close();
};

