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
	GLenum target;
	GLenum slot;
public:
	GLuint id;
	Texture(const char* texture_path, GLenum slot, GLenum texture_target, GLenum internal_format, GLenum format, GLenum pixel_type);
	Texture(int width, int height, GLenum internal_format, GLenum format, GLenum pixel_type,
		GLenum wrap_type,GLenum min_mag_filter_type);
	void activate();
	void bind();
	void unbind();
	void destroy();
};
