#version 330

layout (location = 0) in vec3 vertex_pos;

uniform mat4 cam_matrix; //projection * view
uniform mat4 cube_model; //model

void main() {
	gl_Position = cam_matrix * cube_model * vec4(vertex_pos, 1.0);
}