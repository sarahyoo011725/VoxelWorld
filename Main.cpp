#include <iostream>
#include "StartScreen.h"
#include "GameScreen.h"

using namespace std;
using namespace glm;

const static int width = 1200;
const static int height = 700;

WindowSetting window_setting;
bool start_game = false;

void resize_window(GLFWwindow *window, int width, int height);
void process_inputs(GLFWwindow *window);
void focus_callback(GLFWwindow* window, int focused);
void on_window_focused(GLFWwindow* window);
void unfocus_window(GLFWwindow* window);

int main() {
	//initializees glfw libraries
	glfwInit();

	//specifies opengl version (3.3)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//gets access to modern features, not outdated ones.
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
	
	//creates window
	GLFWwindow* window = glfwCreateWindow(width, height, "Voxel World", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to create a window" << endl;
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwFocusWindow(window);
	glfwSetWindowFocusCallback(window, focus_callback);
	glfwSetFramebufferSizeCallback(window, resize_window);
	
	gladLoadGL();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	window_setting = { window, width, height, true };
	StartScreen start_screen = StartScreen();
	GameScreen game_screen = GameScreen(&window_setting);

	while (!glfwWindowShouldClose(window)) {
		process_inputs(window);

		if (start_game) {
			game_screen.gl_settings();
			game_screen.draw();
		}
		else {
			start_screen.draw(); 
		}
		
		glfwSwapBuffers(window); //swap the color buffer and displays its output to the screen
		glfwPollEvents(); //checks if any events triggered
	}

	//terminates the program, destroying everything including window
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void resize_window(GLFWwindow *window, int width, int height) {
	//tells glad how to display data and coordinates to the window
	glViewport(0, 0, width, height);
}

void process_inputs(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		unfocus_window(window);
	}
	if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
		start_game = true;
	}
	if (GLFW_HOVERED && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		on_window_focused(window);
	}
}

void focus_callback(GLFWwindow* window, int focused) {
	if (focused) {
		on_window_focused(window);
	}
	else {
		unfocus_window(window);
	}
}

void on_window_focused(GLFWwindow* window) {
	window_setting.window_active = true;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void unfocus_window(GLFWwindow* window) {
	window_setting.window_active = false;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}