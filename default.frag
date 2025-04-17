#version 330 core

in vec2 m_texture_coord;
uniform sampler2D texture_data;

out vec4 FragColor;

void main() {
	FragColor = texture(texture_data, m_texture_coord);
}