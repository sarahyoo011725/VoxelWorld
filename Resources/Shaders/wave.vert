#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 texture_coord;

uniform mat4 cam_matrix;
uniform float time;
uniform vec3 cam_pos;
uniform float fog_start;
uniform float fog_end;

out vec2 tex_coord;
out float fog_factor;

void main() {
	vec3 pos = vertex_pos;
	pos.y -= 0.15;
	pos.y += sin(pos.x * 0.6 + time) * 0.07 + sin(pos.z * 0.6 + time * 0.8) * 0.07;
	gl_Position = cam_matrix * vec4(pos, 1.0);
	tex_coord = texture_coord;
	float dist = distance(pos, cam_pos);
	fog_factor = clamp((fog_end - dist) / (fog_end - fog_start), 0.0, 1.0);
}