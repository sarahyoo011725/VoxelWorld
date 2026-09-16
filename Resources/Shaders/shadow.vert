#version 330 core

layout (location = 0) in vec3 vertex_pos;

uniform mat4 light_space_matrix;

void main() {
	gl_Position = light_space_matrix * vec4(vertex_pos, 1.0);
}
