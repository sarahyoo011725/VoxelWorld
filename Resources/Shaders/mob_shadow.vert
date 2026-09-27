#version 330 core

//mob.vert's placement, for the shadow map
layout (location = 0) in vec3 vertex_pos;
layout (location = 3) in float vertex_part;

uniform samplerBuffer part_matrices;
uniform int first_texel;
uniform int texels_per_mob;
uniform mat4 light_space_matrix;

void main() {
	int m = first_texel + gl_InstanceID * texels_per_mob + int(vertex_part + 0.5) * 4;
	mat4 model = mat4(texelFetch(part_matrices, m), texelFetch(part_matrices, m + 1),
		texelFetch(part_matrices, m + 2), texelFetch(part_matrices, m + 3));
	gl_Position = light_space_matrix * model * vec4(vertex_pos, 1.0);
}
