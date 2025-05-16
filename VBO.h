#pragma once
#include <glad/glad.h>
#include "Block.h"

using namespace std;

class VBO
{
public:
	GLuint id;
	VBO(vector<vertex> vertices, GLsizeiptr size);
	void bind();
	void unbind();
	void destroy();
};

