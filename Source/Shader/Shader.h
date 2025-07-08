#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <string>

using namespace std;
using namespace glm;

string read_file(const char* filename);
void check_errors(GLuint vertex, GLuint fragment, GLuint program);

/*
	a class that organizes implementation of OpenGL shader
*/
class Shader
{
private:
	GLuint id;
public:
	Shader(const char* vertex_file, const char* fragment_file);
	GLuint get_id();
	void activate();
	void set_uniform_1i(const char* uniform_name, GLint value);
	void set_uniform_1f(const char* uniform_name, GLfloat value);
	void set_uniform_4f(const char* uniform_name, GLsizei count, vec4 value);
	void set_uniform_mat4f(const char* uniform_name, GLsizei count, GLboolean tranpose, mat4 value);
	void destroy();
};