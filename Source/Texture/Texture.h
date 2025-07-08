#pragma once
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <map>
#include <glm/glm.hpp>
#include "Shader/Shader.h"

using namespace std;
using namespace glm;

/*
	a class that organizes implementation of OpenGL texture
*/
class Texture
{
private:
	GLuint id;
	GLenum target;
	GLenum slot;
public:
	Texture(const char* texture_path, GLenum slot, GLenum texture_target, GLenum internal_format, GLenum format, GLenum pixel_type);
	Texture(int width, int height, GLenum internal_format, GLenum format, GLenum pixel_type,
		GLenum wrap_type,GLenum min_mag_filter_type);
	GLuint get_id();
	void activate();
	void bind();
	void unbind();
	void destroy();
	~Texture() {
		destroy();
	}
};
