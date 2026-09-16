#version 330 core

out vec4 FragColor;

in vec2 tex_coord;
in float fog_factor;

uniform sampler2D texture1;
uniform vec3 fog_color;

void main() {
	vec4 tex_color = texture(texture1, tex_coord);
	if (tex_color.a < 0.1) discard;
	FragColor = mix(vec4(fog_color, 1.0), tex_color, fog_factor);
}
