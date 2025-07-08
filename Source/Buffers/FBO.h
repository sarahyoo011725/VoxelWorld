#pragma once
#include <glad/glad.h>

/*
	creates a post-processing frame buffer
*/
class FBO {
public:
	GLuint id;
	FBO();
	void bind();
	void unbind();
	void attach_texture(GLenum attachment, GLuint texture_id);
	void attach_render_buffer(GLenum attachment, GLuint rbo_id);
	void destroy();
	~FBO() {
		destroy();
	}
};