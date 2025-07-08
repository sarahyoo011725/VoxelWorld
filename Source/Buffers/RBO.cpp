#include "RBO.h"

RBO::RBO(int width, int height, GLenum internal_format) {
	glGenRenderbuffers(1, &id);
	glBindRenderbuffer(GL_RENDERBUFFER, id);
	glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

GLuint RBO::get_id()
{
	return id;
}

void RBO::bind() {
	glBindRenderbuffer(GL_RENDERBUFFER, id);
}

void RBO::unbind() {
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void RBO::destroy()
{
	glDeleteRenderbuffers(1, &id);
}
