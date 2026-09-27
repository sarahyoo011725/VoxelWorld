#version 330 core

//one mob type drawn instanced; each instance's part matrices come from a texture buffer
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 vertex_uv;
layout (location = 3) in float vertex_part;

uniform samplerBuffer part_matrices;
uniform int first_texel;
uniform int texels_per_mob; //4 per part, then one holding the mob's daylight and hurt flash
uniform mat4 cam_matrix;
uniform vec3 cam_pos;
uniform float fog_start;
uniform float fog_end;

//default.frag's inputs: with tex_coord at 0 its atlas lookup lands exactly on tile_origin
out vec2 tex_coord;
out vec2 tile_origin;
out vec3 tint;
out float block_light;
out float block_glow;
out float fog_factor;
out vec3 world_pos;
out vec3 normal;

void main() {
	int base = first_texel + gl_InstanceID * texels_per_mob;
	int m = base + int(vertex_part + 0.5) * 4;
	mat4 model = mat4(texelFetch(part_matrices, m), texelFetch(part_matrices, m + 1),
		texelFetch(part_matrices, m + 2), texelFetch(part_matrices, m + 3));

	vec4 world = model * vec4(vertex_pos, 1.0);
	gl_Position = cam_matrix * world;
	tex_coord = vec2(0.0);
	tile_origin = vertex_uv;
	vec4 extra = texelFetch(part_matrices, base + texels_per_mob - 1);
	tint = mix(vec3(1.0), vec3(1.0, 0.45, 0.45), extra.y);
	block_light = extra.x;
	block_glow = 0.0;
	world_pos = world.xyz;
	normal = mat3(model) * vertex_normal;
	float dist = distance(world.xyz, cam_pos);
	fog_factor = clamp((fog_end - dist) / (fog_end - fog_start), 0.0, 1.0);
}
