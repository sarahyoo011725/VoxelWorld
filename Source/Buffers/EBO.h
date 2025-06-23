#pragma once
#include <glad/glad.h>

using namespace std;

/*
	a class organizing EBO buffer setups
*/
class EBO
{
public:
	GLuint id;
	EBO(const void* indices, GLsizeiptr size, GLenum draw_type);
	void reset_indices(const void* indices, GLsizeiptr size, GLenum draw_type);
	void sub_vertices(const void* indices, GLsizeiptr size, GLsizeiptr offset);
	void bind();
	void unbind();
	void destroy();
};