#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 texture_coord;
layout (location = 2) in vec3 vertex_normal;
layout (location = 3) in vec2 vertex_tile_origin;
layout (location = 4) in vec3 vertex_tint;

uniform mat4 cam_matrix; //projection * view
uniform vec3 cam_pos;
uniform float fog_start;
uniform float fog_end;

out vec2 tex_coord;
out vec2 tile_origin;
out vec3 tint;
out float fog_factor;
out vec3 world_pos;
out vec3 normal;

void main() {
	//gl_Position must always be this order: projection * view * model * vec4(vertex_pos, 1.0)
	gl_Position = cam_matrix * vec4(vertex_pos, 1.0);
	tex_coord = texture_coord;
	tile_origin = vertex_tile_origin;
	tint = vertex_tint;
	world_pos = vertex_pos;
	normal = vertex_normal;
	float dist = distance(vertex_pos, cam_pos);
	fog_factor = clamp((fog_end - dist) / (fog_end - fog_start), 0.0, 1.0);
}
