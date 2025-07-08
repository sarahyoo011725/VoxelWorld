#include "VBO.h"

/*
	creates a VBO instance, binding vertices to it
*/
VBO::VBO(const void* vertices, GLsizeiptr size, GLenum draw_type) {
	glGenBuffers(1, &id);
	glBindBuffer(GL_ARRAY_BUFFER, id);
	glBufferData(GL_ARRAY_BUFFER, size, vertices, draw_type);
}

GLuint VBO::get_id()
{
	return id;
}

/*
	replaces the whole data with a new vertices data
*/
void VBO::reset_vertices(const void* vertices, GLsizeiptr size, GLenum draw_type) {
	bind();
	glBufferData(GL_ARRAY_BUFFER, size, vertices, draw_type);
}

/*
	inserts a new vertices data at the provided offset
*/
void VBO::sub_vertices(const void* vertices, GLsizeiptr size, GLsizeiptr offset) {
	bind();
	glBufferSubData(GL_ARRAY_BUFFER, offset, size, vertices);
}

/*
	binds a VBO
*/
void VBO::bind() {
	glBindBuffer(GL_ARRAY_BUFFER, id);
}

/*
	unbinds a VBO
*/
void VBO::unbind() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
	destroys a VBO instance
*/
void VBO::destroy() {
	glDeleteBuffers(1, &id);
}