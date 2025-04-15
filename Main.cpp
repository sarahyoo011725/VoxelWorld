#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

using namespace std;

const unsigned int width = 1200;
const unsigned int height = 700;

float vertices[] = {
//	x		y		z
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
	0.0f, 0.5f, 0.0f
};

const char* shaderCode =  "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
	"}\0";

const char* fragShaderCode = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
	"}\n\0";

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


	//vertex shader to load verticies?
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &shaderCode, NULL);
	glCompileShader(vertexShader);

	//check if shader compilation was successful
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		cout << "ERROR: vertex shader compilation failed\n" << infoLog << endl;
	}

	//fragment shader for color
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragShaderCode, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		cout << "ERROR: fragment shader compilation failed\n" << infoLog << endl;
	}

	unsigned int program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		cout << "ERROR: shader program link failed\n" << infoLog << endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//TODO: create VAO class
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//TODO: create VBO class
	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//tells OpenGL how it should interpret vertex data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	while (!glfwWindowShouldClose(window)) {
		process_input(window);
		glClearColor((GLfloat)135/255, (GLfloat)206/255, (GLfloat)235/255, 1.0); //add sky color
		glClear(GL_COLOR_BUFFER_BIT);

		//draws a triangle
		glUseProgram(program);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices));

		glfwSwapBuffers(window); //swap the color buffer and displays its output to the screen
		glfwPollEvents(); //checks if any events triggered
	}

	//terminates the program, destroying all components including window and cursors.
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(program);
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