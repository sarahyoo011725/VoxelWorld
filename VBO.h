#pragma once
#include <glad/glad.h>
#include "Block.h"

using namespace std;

/*
Technical Note:
	It is better to manually initialize vbo and ebo with their array byte size as a parameter.
	Specifying their size in constructor by default causes some byte allocating issue, which makes nothing drawn on the window.
	To calculate byte size of array or vector:
	- vector: sizeof(element type) * vector.size()
	- array: sizeof(array)
	To calculate array length in the other way:
	- sizeof(array) / sizeof(element type)
*/

class VBO
{
public:
	GLuint id;
	VBO(const void* vertices, GLsizeiptr size, GLenum draw_type);
	void reset_vertices(const void* vertices, GLsizeiptr size, GLenum draw_type);
	void bind();
	void unbind();
	void destroy();
};