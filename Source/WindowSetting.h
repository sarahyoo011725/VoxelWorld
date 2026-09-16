#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct WindowSetting {
	GLFWwindow* window;
	int width;
	int height;
	bool window_active;
	double scroll_delta_y; 
};
