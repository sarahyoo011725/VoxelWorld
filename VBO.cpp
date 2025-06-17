#ifndef VBO_CLASS_H
#define VBO_CLASS_H

#include "VBO.h"

VBO::VBO(const void* vertices, GLsizeiptr size, GLenum draw_type) {
	glGenBuffers(1, &id);
	glBindBuffer(GL_ARRAY_BUFFER, id);
	glBufferData(GL_ARRAY_BUFFER, size, vertices, draw_type);
}

void VBO::reset_vertices(const void* vertices, GLsizeiptr size, GLenum draw_type) {
	bind();
	glBufferData(GL_ARRAY_BUFFER, size, vertices, draw_type);
}

void VBO::bind() {
	glBindBuffer(GL_ARRAY_BUFFER, id);
}

void VBO::unbind() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::destroy() {
	glDeleteBuffers(1, &id);
}

#endif