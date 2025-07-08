#include "Texture.h"

/*
	creates a texture with an image
*/
Texture::Texture(const char* filename, GLenum slot, GLenum texture_target, GLenum internal_format, GLenum format, GLenum pixel_type) {
	this->slot = slot;
	this->target = texture_target;
	glGenTextures(1, &id);
	glActiveTexture(slot);
	glBindTexture(texture_target, id);

	int width, height, channels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);

	if (data) {
		glTexImage2D(texture_target, 0, internal_format, width, height, 0, format, pixel_type, data);
		glGenerateMipmap(texture_target);
	}
	else {
		cout << "Failed to load data" << endl;
	}

	glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(texture_target, 0);
	stbi_image_free(data);
}

/*
	creates a texture color buffer
*/
Texture::Texture(int width, int height, GLenum internal_format, GLenum format, GLenum pixel_type,
	GLenum wrap_type, GLenum min_mag_filter_type) {
	target = GL_TEXTURE_2D;
	this->slot = -1;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, pixel_type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_mag_filter_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, min_mag_filter_type);
	glBindTexture(GL_TEXTURE_2D, 0);
}

/*
	activates a texture
*/
void Texture::activate() {
	glActiveTexture(slot);
}

/*
	binds a texture
*/
void Texture::bind() {
	glBindTexture(target, id);
}

/*
	unbinds a texture
*/
void Texture::unbind() {
	glBindTexture(target, 0);
}

/*
	destroys a texture instance
*/
void Texture::destroy() {
	glDeleteTextures(1, &id);
}
