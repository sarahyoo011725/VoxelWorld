#pragma once
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <map>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

class Texture
{
public:
	GLuint id;
	Texture(const char* texture_path);
	void bind();
	void unbind();
	void destroy();
};
