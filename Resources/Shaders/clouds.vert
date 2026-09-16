#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 texture_coord;
layout (location = 2) in vec3 vertex_normal;

uniform mat4 cam_matrix;
uniform float time;
uniform float wind_speed;

out vec3 normal;

void main() {
	vec3 pos = vertex_pos;
	pos.x += time * wind_speed;
	gl_Position = cam_matrix * vec4(pos, 1.0);
	normal = vertex_normal;
}
