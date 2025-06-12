#version 330 core

layout (location = 0) in vec3 pos;

uniform mat4 cam_matrix;
uniform float scale;

out vec2 tex_coord;

void main() {
	vec3 crntPos = pos * scale;
	gl_Position = cam_matrix * vec4(crntPos, 1.0f);
}	