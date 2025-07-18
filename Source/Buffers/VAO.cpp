#include "VAO.h"

/*
	creates a VAO instance
*/
VAO::VAO() {
	glGenVertexArrays(1, &id);
}

GLuint VAO::get_id()
{
	return id;
}

/*
	links VBO attributes to the current VAO.

* length: the length of the current attribute
* stride: the amount of bytes or distance from the beginning element of an attribute to the beginning element of the current attribute 
(if stride is 0, it means attributes are tightly packed)
* offset: distance from the beginning element among all attributes to the beginning element of the current attribute
*/
void VAO::link_attrib(VBO& VBO, GLuint layout, GLint length, GLenum type, GLboolean normalized, GLsizei stride, const void *offset) {
	VBO.bind();
	glVertexAttribPointer(layout, length, type, normalized, stride, offset);
	glEnableVertexAttribArray(layout);
	VBO.unbind();
}

/*
	binds a VAO
*/
void VAO::bind() {
	glBindVertexArray(id);
}

/*
	binds a VAO
*/
void VAO::unbind() {
	glBindVertexArray(0);
}

/*
	destroys a VAO instance
*/
void VAO::destroy() {
	glDeleteVertexArrays(1, &id);
}