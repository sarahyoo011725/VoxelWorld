#version 330 core

in vec3 normal;
out vec4 FragColor;

uniform vec3 light_color;

void main() {
	float face_brightness = 0.8 + normal.y * 0.2;
	vec3 cloud_color = vec3(1.0) * light_color * face_brightness;
	FragColor = vec4(cloud_color, 0.9);
}
