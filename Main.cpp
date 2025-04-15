#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

const unsigned int width = 1200;
const unsigned int height = 700;

void resize_window(GLFWwindow* window, int width, int height);
void process_input(GLFWwindow* window);

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
	glfwSetFramebufferSizeCallback(window, resize_window);
	
	gladLoadGL();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	while (!glfwWindowShouldClose(window)) {
		process_input(window);
		glClearColor((GLfloat)135/255, (GLfloat)206/255, (GLfloat)235/255, 1.0); //add sky color
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(window); //swap the color buffer and displays its output to the screen
		glfwPollEvents(); //checks if any events triggered
	}

	//terminates the program, destroying all components including window and cursors.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void resize_window(GLFWwindow* window, int width, int height) {
	//tells glad how to display data and coordinates to the window
	glViewport(0, 0, width, height);
}

void process_input(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}