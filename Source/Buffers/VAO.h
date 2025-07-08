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
	void bind();
	void unbind();
	void destroy();
};