#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 texture_coord;
layout (location = 2) in vec3 vertex_normal;

uniform mat4 cam_matrix; 
uniform float time;

out vec2 tex_coord;

void main() {
	vec3 pos = vertex_pos;
	pos.y -= 0.15;
	pos.y += sin(pos.x + time) * 0.12;
	gl_Position = cam_matrix * vec4(pos, 1.0);
	tex_coord = texture_coord;
}	