#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 texture_coord;
layout (location = 2) in vec3 vertex_normal;

uniform mat4 cam_matrix; //projection * view

out vec2 tex_coord;

void main() {
	//gl_Position must always be this order: projection * view * model * vec4(vertex_pos, 1.0)
	gl_Position = cam_matrix * vec4(vertex_pos, 1.0);
	tex_coord = texture_coord;
}	