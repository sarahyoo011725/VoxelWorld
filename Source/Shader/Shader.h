#pragma once
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <string>

using namespace std;

string read_file(const char* filename);
void check_errors(GLuint vertex, GLuint fragment, GLuint program);

/*
	a class that organizes implementation of opengl shader
*/
class Shader
{
public:
	GLuint id;
	Shader(const char* vertex_file, const char* fragment_file);
	void activate();
	void set_uniform_1i(const char* uniform_name, GLuint value);
	void destroy();
};