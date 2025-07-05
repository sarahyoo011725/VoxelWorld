#pragma once
#include <glad/glad.h>
#include "Block/Block.h"

using namespace std;

/*
	Byte size note:
	- vector: sizeof(element type) * vector.size()
	- array: sizeof(array)
	To calculate array length in the other way:
	- sizeof(array) / sizeof(element type)
*/

/*
	a class that organizes implementation of OpenGL VBO buffer
*/
class VBO
{
public:
	GLuint id;
	VBO(const void* vertices, GLsizeiptr size, GLenum draw_type);
	void reset_vertices(const void* vertices, GLsizeiptr size, GLenum draw_type);
	void sub_vertices(const void* vertices, GLsizeiptr size, GLsizeiptr offset);
	void bind();
	void unbind();
	void destroy();
};