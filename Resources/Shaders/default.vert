#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in uint vertex_data; //packed, see block_vertex in Block.h
layout (location = 2) in vec4 vertex_tint;

uniform mat4 cam_matrix; //projection * view
uniform vec3 cam_pos;
uniform float fog_start;
uniform float fog_end;

out vec2 tex_coord;
out vec2 tile_origin;
out vec3 tint;
out float block_light;
out float block_glow;
out float fog_factor;
out vec3 world_pos;
out vec3 normal;

const vec3 directions[6] = vec3[](vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, -1, 0), vec3(0, 0, 1), vec3(0, 0, -1));
const vec2 tile_size = vec2(1.0 / 16.0, 1.0 / 34.0);

void main() {
	gl_Position = cam_matrix * vec4(vertex_pos, 1.0);
	normal = directions[vertex_data & 7u];
	bool glows = ((vertex_data >> 11) & 1u) == 1u;
	//above 1 tells the fragment shader the block lights itself
	block_light = glows ? 2.0 : float((vertex_data >> 3) & 15u) / 15.0;
	block_glow = float((vertex_data >> 7) & 15u) / 15.0;
	tex_coord = vec2(float((vertex_data >> 12) & 31u), float((vertex_data >> 17) & 31u));
	//the atlas is flipped on load, so row 0 of the image is the top of v
	tile_origin = vec2(float((vertex_data >> 22) & 15u), 33.0 - float((vertex_data >> 26) & 63u)) * tile_size;
	tint = vertex_tint.rgb;
	world_pos = vertex_pos;
	float dist = distance(vertex_pos, cam_pos);
	fog_factor = clamp((fog_end - dist) / (fog_end - fog_start), 0.0, 1.0);
}
