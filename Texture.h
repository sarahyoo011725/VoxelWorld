#pragma once

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

using namespace std;

class Texture
{
public:
	GLuint id;
	Texture(const char* texture_path, int width, int height, int channels);
	void bind();
	void unbind();
	void destroy();
};