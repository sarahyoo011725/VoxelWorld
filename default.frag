#version 330 core

out vec4 FragColor;

in vec2 tex_coord;

uniform sampler2D texture_data;

void main() {
	FragColor = texture(texture_data, tex_coord);
}