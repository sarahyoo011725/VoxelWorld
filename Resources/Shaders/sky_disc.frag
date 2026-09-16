#version 330 core

in vec2 local_pos;
out vec4 FragColor;

uniform vec3 color; //edge color (core mode) or glow color (glow mode)
uniform vec3 core_color; //bright center color, core mode only
uniform int is_glow;

void main() {
	float d = length(local_pos);

	if (is_glow == 1) {
		//a soft circular halo behind the disc - the bloom look shader packs give the sun/moon
		if (d > 1.0) discard;
		float alpha = pow(1.0 - d, 2.0) * 0.5;
		FragColor = vec4(color, alpha);
		return;
	}

	//flat square silhouette (like Minecraft's sun/moon), but with a bright core fading
	//toward the edge so it doesn't look like a single flat color swatch
	float t = clamp(d, 0.0, 1.0);
	vec3 tint = mix(core_color, color, t);
	FragColor = vec4(tint, 1.0);
}
