#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texture_coord;

uniform mat4 view;

out vec2 m_texture_coord;

void main() {
	gl_Position = view * vec4(pos, 1.0);
	m_texture_coord = texture_coord;
}	