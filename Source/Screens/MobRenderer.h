#pragma once
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <vector>
#include "Buffers/VAO.h"
#include "Buffers/VBO.h"
#include "Entity/Mob.h"
#include "Shader/ShaderManager.h"
#include "Texture/Texture.h"

using namespace glm;

struct mob_vertex {
	vec3 position; //blocks, relative to the part's pivot
	vec3 normal;
	vec2 uv; //into the type's skin
	float part;
};

/*
	each mob type's mesh is built once from its model and stays on the GPU; a
	frame only uploads where every part of every mob is (a matrix per part, in a
	texture buffer) and draws each type in one instanced call
*/
class MobRenderer {
public:
	MobRenderer();
	~MobRenderer();
	//extra: one more mob outside the list, e.g. the player seen riding in third person
	void build(const vector<unique_ptr<Mob>>& mobs, const Mob* extra = nullptr);
	void draw();
	void draw_depth(const mat4& light_space_matrix);

	static const int skin_unit = 5;
	static const int matrix_unit = 6;

private:
	struct TypeMesh {
		VAO vao;
		VBO vbo = VBO(nullptr, 0, GL_STATIC_DRAW);
		unique_ptr<Texture> skin;
		GLsizei vertex_count = 0;
		int texels_per_mob = 0;
		int first_texel = 0;
		int instances = 0;
	};

	TypeMesh& mesh_for(const MobType& type);
	void pose(const Mob& mob, vector<vec4>& out) const;
	void draw_all(Shader& shader, bool with_skin);

	ShaderManager& sm;
	map<const MobType*, unique_ptr<TypeMesh>> meshes;
	vector<vec4> texels;
	GLuint matrix_buffer = 0;
	GLuint matrix_texture = 0;
};
