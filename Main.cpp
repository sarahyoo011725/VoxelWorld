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

using namespace std;

const unsigned int width = 1200;
const unsigned int height = 700;

bool wireframe = false;
bool isWindowActive = true;

//cube verticies
GLfloat vertices[] = { 
//    x     y      z      texture
	//front
	-0.5, 0.5, 0.5,		0.0, 1.0,	// Front Top Left		
	0.5,  0.5, 0.5,		1.0, 1.0,	// Front Top Right	
	0.5, -0.5, 0.5,		1.0, 0.0,	// Front Bottom Right	
	-0.5,-0.5, 0.5,		0.0, 0.0,	// Front Bottom Left
	//back
	-0.5, 0.5,-0.5,		0.0, 1.0,	// Back Top Left
	0.5, 0.5, -0.5,		1.0, 1.0, 	// Back Top Right
	0.5,-0.5, -0.5,		1.0, 0.0,	// Back Bottom Right
	-0.5,-0.5,-0.5,		0.0, 0.0,	// Back Bottom Left		
	//left
	-0.5, 0.5, 0.5,		0.0, 1.0,
	-0.5,0.5, -0.5, 	1.0, 1.0,
	-0.5,-0.5,-0.5,		1.0, 0.0,
	-0.5, -0.5, 0.5,	0.0, 0.0,
	//right
	0.5, 0.5, 0.5,		0.0, 1.0,
	0.5, 0.5, -0.5,		1.0, 1.0,
	0.5,-0.5, -0.5,		1.0, 0.0,
	0.5, -0.5, 0.5,		0.0, 0.0,
	//top
	-0.5, 0.5, -0.5,	0.0, 1.0,
	0.5, 0.5, -0.5,		1.0, 1.0,
	0.5, 0.5, 0.5,		1.0, 0.0,
	-0.5, 0.5, 0.5,		0.0, 0.0,
	//bottom
	-0.5, -0.5, -0.5,	0.0, 1.0,
	0.5, -0.5, -0.5,	1.0, 1.0,
	0.5, -0.5, 0.5,		1.0, 0.0,
	-0.5, -0.5, 0.5,	0.0, 0.0,
};
//cube indices
GLuint indices[] = {
//front
	0,1,2,  
	2,3,0,
//back
	4,5,6,	
	6,7,4,
//left
	8,9,10,
	10,11,8,
//right
	12,13,14,
	14,15,12,
//top
	16,17,18,
	18,19,16,
//bottom
	20,21,22,
	22,23,20,
};

void resize_window(GLFWwindow *window, int width, int height);
void process_inputs(GLFWwindow *window);
void focus_callback(GLFWwindow* window, int focused);

void on_window_focused(GLFWwindow* window);
void unfocus_winow(GLFWwindow* window);

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

	Shader shader("default.vert", "default.frag");
	Texture texture("dirt.jpg", 1, 1, 1);
	VAO vao;
	VBO vbo(vertices, sizeof(vertices));
	EBO ebo(indices, sizeof(indices));

	vao.bind();
	vao.link_attrib(vbo, 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); //vertex positions
	vao.link_attrib(vbo, 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); //texture coordinates
	
	//unbind all to prevent accidentally motifying them
	vao.unbind();
	vbo.unbind();
	ebo.unbind();

	glEnable(GL_DEPTH_TEST);

	Camera cam(window, width, height);
	while (!glfwWindowShouldClose(window)) {
		process_inputs(window);
		if (isWindowActive) {
			cam.process_inputs();
		}
		cam.update_view(shader);
		glClearColor((GLfloat)135/255, (GLfloat)206/255, (GLfloat)235/255, 1.0); //add sky color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		//draws a rectangle
		shader.activate();
		texture.bind();
		vao.bind();
		ebo.bind();
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(GLuint), GL_UNSIGNED_INT, 0);
		
		glfwSwapBuffers(window); //swap the color buffer and displays its output to the screen
		glfwPollEvents(); //checks if any events triggered
	}

	//terminates the program, destroying everything including window
	vao.destroy();
	vbo.destroy();
	ebo.destroy();
	texture.destroy();
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
		unfocus_winow();
	}
	if (GLFW_HOVERED && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
		glfwFocusWindow(window);
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
		unfocus_winow(window);
	}
}

void on_window_focused(GLFWwindow* window) {
	isWindowActive = false;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void unfocus_winow(GLFWwindow* window) {
	isWindowActive = false;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}