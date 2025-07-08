#include "FBO.h"

FBO::FBO() {
	glGenFramebuffers(1, &id);
}

void FBO::bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void FBO::unbind() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::attach_texture(GLenum attachment, GLuint texture_id) {
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture_id, 0);
}

void FBO::attach_render_buffer(GLenum attachment, GLuint rbo_id) {
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo_id);
}

void FBO::destroy() {
	glDeleteFramebuffers(1, &id);
}