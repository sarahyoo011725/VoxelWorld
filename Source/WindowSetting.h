#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

struct WindowSetting {
	GLFWwindow* window;
	int width;
	int height;
	bool window_active;
	double scroll_delta_y;
	std::string typed_text; //printable characters typed since the console last read them
};
