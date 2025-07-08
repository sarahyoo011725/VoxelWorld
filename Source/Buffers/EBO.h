#pragma once
#include <glad/glad.h>

using namespace std;

/*
	a class organizing EBO buffer setups
*/
class EBO
{
private:
	GLuint id;
public:
	EBO(const void* indices, GLsizeiptr size, GLenum draw_type);
	GLuint get_id();
	void reset_indices(const void* indices, GLsizeiptr size, GLenum draw_type);
	void sub_vertices(const void* indices, GLsizeiptr size, GLsizeiptr offset);
	void bind();
	void unbind();
	void destroy();
};