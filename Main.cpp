#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

using namespace std;

const unsigned int width = 1200;
const unsigned int height = 700;

bool wireframe = false;

//rectangle verticies
GLfloat vertices[] = { 
//	x		y		z
	-0.5f,  0.5f, 0.0f,   // top left 
	0.5f,  0.5f, 0.0f,  // top right
	-0.5f, -0.5f, 0.0f,  // bottom left
	0.5f, -0.5f, 0.0f,  // bottom right
};

GLuint indices[] = {
	0, 1, 3,
	0, 2, 3
};

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

	Shader shader("default.vert", "default.frag");
	VAO vao;
	VBO vbo(vertices, sizeof(vertices));
	EBO ebo(indices, sizeof(indices));

	vao.bind();
	vao.link_VBO(vbo, 0);
	//unbind all to prevent accidentally motifying them
	vao.unbind();
	vbo.unbind();
	ebo.unbind();

	while (!glfwWindowShouldClose(window)) {
		process_input(window);
		glClearColor((GLfloat)135/255, (GLfloat)206/255, (GLfloat)235/255, 1.0); //add sky color
		glClear(GL_COLOR_BUFFER_BIT);

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		//draws a rectangle
		shader.activate();
		vao.bind();
		ebo.bind();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		glfwSwapBuffers(window); //swap the color buffer and displays its output to the screen
		glfwPollEvents(); //checks if any events triggered
	}

	//terminates the program, destroying everything including window
	vao.destroy();
	vbo.destroy();
	ebo.destroy();
	shader.destroy();
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
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		wireframe = !wireframe;
	}
}