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
	a class that organizes implementation of opengl texture
*/
class Texture
{
private:
	GLenum type;
public:
	GLuint id;
	Texture(const char* texture_path, GLenum texture_type, GLenum slot, GLenum format, GLenum pixel_type);
	void set_unit(Shader shader, const char* uniform, GLuint unit);
	void activate(GLenum texture);
	void bind();
	void unbind();
	void destroy();
};
