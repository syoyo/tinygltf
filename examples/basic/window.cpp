#include "window.h"



Window::Window(int x, int y, const char* title)
{
	GLFWwindow* window = glfwCreateWindow(x, y, title, NULL, NULL);
	this->window = window;
}


Window::~Window()
{
	glfwDestroyWindow(this->window);
}

void Window::Resize() 
{
	GLint w, h;
	glfwGetWindowSize(this->window, &w, &h);
	glViewport(0, 0, w, h);
}

int Window::Close()
{
	return glfwWindowShouldClose(this->window);
}
