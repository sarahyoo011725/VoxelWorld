#version 330 core

out vec4 FragColor;

in vec2 tex_coord;
in float fog_factor;
in vec3 world_pos;
in vec3 normal;

uniform sampler2D texture1;
uniform sampler2D shadow_map;
uniform vec3 fog_color;
uniform vec3 sun_direction; //direction the sunlight travels, normalized
uniform mat4 light_space_matrix;

//1.0 = fully in shadow, 0.0 = fully lit
float calculate_shadow(vec3 n) {
	vec4 light_space_pos = light_space_matrix * vec4(world_pos, 1.0);
	vec3 proj = light_space_pos.xyz / light_space_pos.w;
	proj = proj * 0.5 + 0.5;
	if (proj.z > 1.0) return 0.0;

	float bias = max(0.004 * (1.0 - dot(n, -sun_direction)), 0.001);
	float shadow = 0.0;
	vec2 texel = 1.0 / textureSize(shadow_map, 0);
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			float closest_depth = texture(shadow_map, proj.xy + vec2(x, y) * texel).r;
			shadow += (proj.z - bias > closest_depth) ? 1.0 : 0.0;
		}
	}
	return shadow / 9.0;
}

void main() {
	vec4 tex_color = texture(texture1, tex_coord);
	if (tex_color.a < 0.1) discard;

	vec3 n = normalize(normal);
	float shadow = calculate_shadow(n);
	float diffuse = max(dot(n, -sun_direction), 0.0);
	float light = 0.45 + diffuse * 0.55 * (1.0 - shadow);

	vec3 lit_color = tex_color.rgb * light;
	FragColor = mix(vec4(fog_color, 1.0), vec4(lit_color, tex_color.a), fog_factor);
}
