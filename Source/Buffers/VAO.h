#pragma once
#include <glad/glad.h>
#include "VBO.h"

/*
	a class that organizes implementation of opengl VAO buffer
*/
class VAO
{
private:
	GLuint id;
public:
	VAO();
	GLuint get_id();
	void link_attrib(VBO& VBO, GLuint layout, GLint length, GLenum type, GLboolean normalized, GLsizei stride, const void *offset);
	//for integer attributes the shader reads as uint/int, which link_attrib would convert to float
	void link_integer_attrib(VBO& VBO, GLuint layout, GLint length, GLenum type, GLsizei stride, const void* offset);
	void bind();
	void unbind();
	void destroy();
};