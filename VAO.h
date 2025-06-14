#pragma once
#include <glad/glad.h>
#include "VBO.h"

class VAO
{
public:
	GLuint id;
	VAO();
	void link_attrib(VBO& VBO, GLuint layout, GLint length, GLenum type, GLboolean normalized, GLsizei stride, const void *offset);
	void bind();
	void unbind();
	void destroy();
};