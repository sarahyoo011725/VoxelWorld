#ifndef EBO_CLASS_H
#define EBO_CLASS_H

#include <glad/glad.h>
#include <vector>

using namespace std;

class EBO
{
public:
	GLuint id;
	EBO(vector<GLuint> indices, GLsizeiptr size);
	void set_indices(vector<GLuint> indices, GLsizeiptr size);
	void bind();
	void unbind();
	void destroy();
};

#endif
