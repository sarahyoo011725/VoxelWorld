#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texture_coord;

uniform mat4 cam_matrix;

out vec2 m_texture_coord;

void main() {
	gl_Position = cam_matrix * vec4(pos, 1.0);
	m_texture_coord = texture_coord;
}	