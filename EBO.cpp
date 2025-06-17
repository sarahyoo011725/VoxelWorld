#include "EBO.h"

EBO::EBO(const void* indices, GLsizeiptr size, GLenum draw_type) {
	glGenBuffers(1, &id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, draw_type);
}

void EBO::reset_indices(const void* indices, GLsizeiptr size, GLenum draw_type) {
	bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, draw_type);
}

void EBO::bind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

void EBO::unbind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::destroy() {
	glDeleteBuffers(1, &id);
}