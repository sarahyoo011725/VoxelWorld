#pragma once
#include <glm/glm.hpp>

using namespace glm;

/*
	the six clipping planes of a view-projection matrix, used to skip chunks that
	cannot appear in the image. works for any such matrix, perspective or
	orthographic, so the same code culls both the camera view and the shadow
	pass's light view.
*/
struct Frustum {
	//xyz is the inward-facing plane normal, w its distance from the origin
	vec4 planes[6];

	/*
		extracts the planes from a projection * view matrix (Gribb-Hartmann).
		each plane is a sum or difference of the matrix's fourth row and one of
		the first three; glm is column major, so a row is gathered across columns
	*/
	void from_matrix(const mat4& m) {
		vec4 row0 = vec4(m[0][0], m[1][0], m[2][0], m[3][0]);
		vec4 row1 = vec4(m[0][1], m[1][1], m[2][1], m[3][1]);
		vec4 row2 = vec4(m[0][2], m[1][2], m[2][2], m[3][2]);
		vec4 row3 = vec4(m[0][3], m[1][3], m[2][3], m[3][3]);

		planes[0] = row3 + row0; //left
		planes[1] = row3 - row0; //right
		planes[2] = row3 + row1; //bottom
		planes[3] = row3 - row1; //top
		planes[4] = row3 + row2; //near
		planes[5] = row3 - row2; //far

		for (int i = 0; i < 6; ++i) {
			float length = glm::length(vec3(planes[i]));
			if (length > 0.0f) planes[i] /= length;
		}
	}

	/*
		tests an axis-aligned box. only the corner furthest along each plane's
		normal has to be checked: if even that one is behind the plane, every
		other corner is too, so the box cannot intersect the frustum.
		may report a box just outside a corner as visible, which only costs a
		wasted draw - it never culls something that should be drawn.
	*/
	bool intersects_aabb(vec3 min_corner, vec3 max_corner) const {
		for (int i = 0; i < 6; ++i) {
			const vec4& plane = planes[i];
			vec3 furthest = vec3(
				plane.x >= 0.0f ? max_corner.x : min_corner.x,
				plane.y >= 0.0f ? max_corner.y : min_corner.y,
				plane.z >= 0.0f ? max_corner.z : min_corner.z
			);
			if (dot(vec3(plane), furthest) + plane.w < 0.0f) return false;
		}
		return true;
	}
};
