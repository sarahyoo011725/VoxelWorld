#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in uint vertex_data; //packed, see block_vertex in Block.h

uniform mat4 cam_matrix;
uniform float time;
uniform vec3 cam_pos;
uniform float fog_start;
uniform float fog_end;

out vec2 tex_coord;
out float fog_factor;
out vec3 world_pos;
out vec3 normal;
out float block_light;
out float block_glow;

const vec3 directions[6] = vec3[](vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, -1, 0), vec3(0, 0, 1), vec3(0, 0, -1));
const vec2 tile_size = vec2(1.0 / 16.0, 1.0 / 34.0);

void main() {
	vec3 pos = vertex_pos;
	pos.y -= 0.15;
	pos.y += sin(pos.x * 0.6 + time) * 0.07 + sin(pos.z * 0.6 + time * 0.8) * 0.07;
	gl_Position = cam_matrix * vec4(pos, 1.0);
	//water faces are never merged, so the corner is 0 or 1 and maps straight into the tile
	vec2 corner = vec2(float((vertex_data >> 12) & 31u), float((vertex_data >> 17) & 31u));
	vec2 tile_origin = vec2(float((vertex_data >> 22) & 15u), 33.0 - float((vertex_data >> 26) & 63u)) * tile_size;
	tex_coord = tile_origin + corner * tile_size;
	world_pos = pos;
	normal = directions[vertex_data & 7u];
	block_light = float((vertex_data >> 3) & 15u) / 15.0;
	block_glow = float((vertex_data >> 7) & 15u) / 15.0;
	float dist = distance(pos, cam_pos);
	fog_factor = clamp((fog_end - dist) / (fog_end - fog_start), 0.0, 1.0);
}
