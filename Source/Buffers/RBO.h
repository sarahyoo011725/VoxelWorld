#pragma once
#include <glad/glad.h>

/*
	Render Buffer Object
*/
class RBO
{
private:
	GLuint id;
public:
	RBO(int width, int height, GLenum internal_format);
	GLuint get_id();
	void bind();
	void unbind();
	void destroy();
};

