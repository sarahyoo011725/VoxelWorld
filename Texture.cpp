#include "Texture.h"

Texture::Texture(const char* filename, GLenum texture_type, GLenum slot, GLenum format, GLenum pixel_type) {
	type = texture_type;
	glGenTextures(1, &id);
	glActiveTexture(slot);
	glBindTexture(texture_type, id);

	int width, height, channels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);

	if (data) {
		glTexImage2D(texture_type, 0, GL_RGBA, width, height, 0, format, pixel_type, data);
		glGenerateMipmap(texture_type);
	}
	else {
		cout << "Failed to load data" << endl;
	}

//	cout << id << endl;

	glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	glBindTexture(texture_type, 0);
	stbi_image_free(data);
}

void Texture::set_unit(Shader shader, const char* uniform, GLuint unit) {
	shader.activate();
	glUniform1i(glGetUniformLocation(shader.id, uniform), unit);
}

void Texture::activate(GLenum texture) {
	glActiveTexture(texture);
}

void Texture::bind() {
	glBindTexture(type, id);
}

void Texture::unbind() {
	glBindTexture(type, 0);
}

void Texture::destroy() {
	glDeleteTextures(1, &id);
}
