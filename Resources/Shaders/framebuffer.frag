#version 330 core

out vec4 FragColor;

in vec2 tex_coord;

uniform sampler2D screen_texture;
uniform sampler2D depth_texture;

uniform int is_underwater;

void main() {
	vec3 fog_color = vec3(0.0, 0.4, 0.7);
	vec3 color = texture(screen_texture, tex_coord).rgb;
	float depth = texture(depth_texture, tex_coord).r;
	float fog_far = 1.2;
	float fog_near = 0.5;

	vec3 final_color = color;

	if (is_underwater == 1) {
		float fog_factor = (fog_far - depth) / (fog_far - fog_near);
		fog_factor = clamp(fog_factor, 0.0, 1.0);
		final_color = mix(fog_color, color, fog_factor);
	}
	
	FragColor = vec4(vec3(final_color), 1.0);
}