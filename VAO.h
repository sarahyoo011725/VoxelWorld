#ifndef VAO_CLASS_H
#define VAO_CLASS_H

#include <glad/glad.h>
#include "VBO.h"

class VAO
{
public:
	GLuint id;
	VAO();
	void link_VBO(VBO& VBO, GLuint layout);
	void bind();
	void unbind();
	void destroy();
};

#endif
