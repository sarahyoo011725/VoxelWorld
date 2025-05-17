#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"
#include "Texture.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Camera.h"
#include "Block.h"
#include "Chunk.h"

using namespace std;
using namespace glm;

const unsigned int width = 1200;
const unsigned int height = 700;

bool wireframe = false;
bool isWindowActive = true;

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
	GLFWwindow* window = glfwCreateWindow(width, height, "VoxelGame", NULL, NULL);
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

	Camera cam(window, width, height, glm::vec3(0.0, 0.0, 3.0));
	Shader shader = Shader("default.vert", "default.frag");
	Chunk chunk = Chunk(100, 100, 100);
	chunk.shader_id = shader.id;
	
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window)) {
		process_inputs(window);
		if (isWindowActive) {
			cam.update();
		}
		cam.update_matrix(shader.id);
		glClearColor((GLfloat)135/255, (GLfloat)206/255, (GLfloat)235/255, 1.0); //add sky color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		
		//draws mesh
		shader.activate();
		chunk.render();

		glfwSwapBuffers(window); //swap the color buffer and displays its output to the screen
		glfwPollEvents(); //checks if any events triggered
	}

	//terminates the program, destroying everything including window
	chunk.destroy();
	shader.destroy();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void resize_window(GLFWwindow *window, int width, int height) {
	//tells glad how to display data and coordinates to the window
	glViewport(0, 0, width, height);
}

void process_inputs(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
		unfocus_window(window);
	}
	if (GLFW_HOVERED && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
		on_window_focused(window);
	}
	if (glfwGetKey(window, GLFW_KEY_1)) {
		wireframe = !wireframe;
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
	isWindowActive = true;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void unfocus_window(GLFWwindow* window) {
	isWindowActive = false;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}