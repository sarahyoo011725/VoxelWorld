#version 330

layout (location = 0) in vec2 vertex_pos;
layout (location = 1) in vec2 texture_coord;

uniform vec2 offset;
uniform vec2 scale;
uniform vec2 uv_offset;
uniform vec2 uv_scale;

out vec2 tex_coord;

void main() {
	gl_Position = vec4(vertex_pos * scale + offset, 0.0, 1.0);
	tex_coord = texture_coord * uv_scale + uv_offset;
}
