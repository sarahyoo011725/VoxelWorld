#version 330 core

layout (location = 0) in vec2 quad_pos; //a -1..1 quad, reused as the local billboard offset

uniform mat4 projection;
uniform mat4 view;
uniform vec3 disc_center;
uniform float disc_size;

out vec2 local_pos;

void main() {
	//extract the camera's world-space right/up axes from the view matrix's rows,
	//so the quad always faces the camera without needing its own model matrix
	vec3 cam_right = vec3(view[0][0], view[1][0], view[2][0]);
	vec3 cam_up = vec3(view[0][1], view[1][1], view[2][1]);
	vec3 world_pos = disc_center + (cam_right * quad_pos.x + cam_up * quad_pos.y) * disc_size;

	gl_Position = projection * view * vec4(world_pos, 1.0);
	local_pos = quad_pos;
}
