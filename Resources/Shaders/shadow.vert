#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 3) in float sway;

uniform mat4 light_space_matrix;
uniform float time;
uniform float sway_scale; //1 for foliage; 0 for solid blocks, whose attribute 3 is their tile origin, not a sway

void main() {
	//matches foliage.vert's wind formula so a leaf/grass shadow moves with its geometry
	vec3 pos = vertex_pos;
	float wind = sin(time * 1.5 + pos.x * 0.8 + pos.z * 0.8) * 0.08;
	pos.x += wind * sway * sway_scale;
	pos.z += wind * sway * sway_scale;
	gl_Position = light_space_matrix * vec4(pos, 1.0);
}
