#include "EBO.h"

/*
	generates an EBO, binding the data to it
*/
EBO::EBO(const void* indices, GLsizeiptr size, GLenum draw_type) {
	glGenBuffers(1, &id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, draw_type);
}

/*
	replaces the whole data with a new indices data
*/
void EBO::reset_indices(const void* indices, GLsizeiptr size, GLenum draw_type) {
	bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, draw_type);
}

/*
	inserts a new indices data at the provided offset
*/
void EBO::sub_vertices(const void* indices, GLsizeiptr size, GLsizeiptr offset) {
	bind();
	glBufferSubData(GL_ARRAY_BUFFER, offset, size, indices);
}

/*
	binds EBO
*/
void EBO::bind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

/*
	unbinds EBO
*/
void EBO::unbind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/*
	destroys EBO buffer
*/
void EBO::destroy() {
	glDeleteBuffers(1, &id);
}