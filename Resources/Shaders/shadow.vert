#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 3) in float sway;

uniform mat4 light_space_matrix;
uniform float time;

void main() {
	//matches foliage.vert's wind formula so a leaf/grass shadow moves with its geometry.
	//opaque blocks have no sway attribute bound, so this attribute reads 0 for them - no-op
	vec3 pos = vertex_pos;
	float wind = sin(time * 1.5 + pos.x * 0.8 + pos.z * 0.8) * 0.08;
	pos.x += wind * sway;
	pos.z += wind * sway;
	gl_Position = light_space_matrix * vec4(pos, 1.0);
}
