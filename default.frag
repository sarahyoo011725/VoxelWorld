#version 330 core

out vec4 FragColor;

in vec2 tex_coord;
in vec3 vert_pos;
in vec3 normal_vec;

uniform sampler2D texture_data;
uniform vec4 light_color;
uniform vec3 light_pos;
uniform vec3 cam_pos;

void main() {

	//calculates light
	// don't care magnitude. direction vector is what needed for light (get unit vector with normalization)
	//use max fuction to prevent dot product from beign ended up negative.
	//--> because lighting is not defined for negative colours

	float ambient = 0.20;
	vec3 norm = normalize(normal_vec);
	vec3 light_dir = normalize(light_pos - vert_pos);
	float diffuse = max(dot(norm, light_dir),0.0f);

	float light_intensity = 0.5f;
	vec3 view_dir = normalize(cam_pos - vert_pos);
	vec3 reflect_dir = reflect(-light_dir, norm);
	float spec_amount = pow(max(dot(view_dir, reflect_dir), 0.0), 8);
	float specular = spec_amount * light_intensity;

	FragColor = texture(texture_data, tex_coord) * light_color * (diffuse + ambient + specular);
}