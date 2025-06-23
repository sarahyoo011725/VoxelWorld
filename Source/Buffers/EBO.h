#pragma once
#include <glad/glad.h>

using namespace std;

class EBO
{
public:
	GLuint id;
	EBO(const void* indices, GLsizeiptr size, GLenum draw_type);
	void reset_indices(const void* indices, GLsizeiptr size, GLenum draw_type);
	void bind();
	void unbind();
	void destroy();
};