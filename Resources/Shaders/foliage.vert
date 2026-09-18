#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 texture_coord;
layout (location = 2) in vec3 vertex_normal;
layout (location = 3) in float sway;
layout (location = 4) in vec3 vertex_tint;

uniform mat4 cam_matrix;
uniform float time;
uniform vec3 cam_pos;
uniform float fog_start;
uniform float fog_end;

out vec2 tex_coord;
out vec3 tint;
out float fog_factor;
out vec3 world_pos;
out vec3 normal;

void main() {
	vec3 pos = vertex_pos;
	float wind = sin(time * 1.5 + pos.x * 0.8 + pos.z * 0.8) * 0.08;
	pos.x += wind * sway;
	pos.z += wind * sway;
	gl_Position = cam_matrix * vec4(pos, 1.0);
	tex_coord = texture_coord;
	tint = vertex_tint;
	world_pos = pos;
	normal = vertex_normal;
	float dist = distance(pos, cam_pos);
	fog_factor = clamp((fog_end - dist) / (fog_end - fog_start), 0.0, 1.0);
}
