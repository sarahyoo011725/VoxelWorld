#version 330 core

layout (location = 0) in vec2 quad_pos; //a -1..1 fullscreen quad in NDC

out vec2 ndc_pos;

void main() {
	gl_Position = vec4(quad_pos, 0.0, 1.0);
	ndc_pos = quad_pos;
}
