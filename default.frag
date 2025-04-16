#version 330 core

uniform vec4 test_color;
out vec4 FragColor;

void main() {
	FragColor = test_color;
}