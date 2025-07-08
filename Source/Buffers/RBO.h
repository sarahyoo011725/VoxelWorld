#pragma once
#include <glad/glad.h>

/*
	Render Buffer Object
*/
class RBO
{
public:
	GLuint id;
	RBO(int width, int height, GLenum internal_format);
	void bind();
	void unbind();
};

