#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texture_coord;
layout (location = 2) in vec3 normal;

uniform mat4 cam_matrix;

out vec2 tex_coord;
out vec3 vert_pos; //vertex position in world space
out vec3 normal_vec;

void main() {
	gl_Position = cam_matrix * vec4(pos, 1.0);
	tex_coord = texture_coord;
	vert_pos = pos; 
	normal_vec = normal;
}	