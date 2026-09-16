#version 330 core

in vec2 ndc_pos;
out vec4 FragColor;

uniform mat4 inv_view;
uniform mat4 inv_projection;
uniform vec3 zenith_color;
uniform vec3 horizon_color;
uniform vec3 to_sun;

const vec3 glow_color = vec3(1.0, 0.55, 0.25);

void main() {
	vec4 clip = vec4(ndc_pos, -1.0, 1.0);
	vec4 view_space = inv_projection * clip;
	view_space /= view_space.w;
	vec3 view_dir = normalize(view_space.xyz);
	vec3 world_dir = normalize((inv_view * vec4(view_dir, 0.0)).xyz);

	float h = clamp(world_dir.y, 0.0, 1.0);
	vec3 col = mix(horizon_color, zenith_color, pow(h, 0.5));

	//a warm glow around the sun's position - brightest at sunrise/sunset since the
	//glow cone sits near the horizon where the viewer is more likely to look toward it
	float sun_dot = max(dot(world_dir, to_sun), 0.0);
	float horizon_closeness = 1.0 - clamp(abs(to_sun.y) * 3.0, 0.0, 1.0);
	col += glow_color * pow(sun_dot, 24.0) * (0.4 + horizon_closeness * 0.6);

	FragColor = vec4(col, 1.0);
}
