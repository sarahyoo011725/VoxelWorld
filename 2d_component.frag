#version 330 core

out vec4 FragColor;

in vec2 tex_coord;

uniform sampler2D texture1;
uniform vec4 color;
uniform int use_texture;

void main() {
	if (use_texture == 1) {		//1 is true in opengl
		FragColor = texture(texture1, tex_coord);
	} else {
		FragColor = color;
	}
}