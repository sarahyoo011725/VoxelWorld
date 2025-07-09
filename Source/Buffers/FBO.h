#pragma once
#include <glad/glad.h>

/*
	creates a frame buffer object
*/
class FBO {
private:
	GLuint id;
public:
	FBO();
	GLuint get_id();
	void bind();
	void unbind();
	void attach_texture(GLenum attachment, GLuint texture_id);
	void attach_render_buffer(GLenum attachment, GLuint rbo_id);
	void draw_buffer(GLenum buffer);
	void read_buffer(GLenum buffer);
	void destroy();
};