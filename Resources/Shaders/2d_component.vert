#version 330

layout (location = 0) in vec2 vertex_pos;
layout (location = 1) in vec2 texture_coord;

out vec2 tex_coord;

void main() {
	gl_Position = vec4(vertex_pos.x, vertex_pos.y, 0.0, 1.0);
	tex_coord = texture_coord;
}