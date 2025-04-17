#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <string>

using namespace std;

string read_file(const char* filename);
void check_errors(GLuint vertex, GLuint fragment, GLuint program);

class Shader
{
public:
	GLuint id;
	Shader(const char* vertex_file, const char* fragment_file);
	void activate();
	void destroy();
};

#endif