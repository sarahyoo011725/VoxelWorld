#include "MobRenderer.h"
#include "Chunk/Chunk.h"
#include "World/ChunkGeneration.h"
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>

namespace {
	const vec3 face_normals[6] = { vec3(1, 0, 0), vec3(0, 0, 1), vec3(-1, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0), vec3(0, -1, 0) };

	//where each face sits in the box's unwrap (x, y, width, height in pixels); see ModelPart
	vec4 face_rect(int face, vec3 size) {
		float w = size.x, h = size.y, d = size.z;
		switch (face) {
		case 0: return vec4(0, d, d, h);
		case 1: return vec4(d, d, w, h);
		case 2: return vec4(d + w, d, d, h);
		case 3: return vec4(2 * d + w, d, w, h);
		case 4: return vec4(d, 0, w, d);
		default: return vec4(d + w, 0, w, d);
		}
	}

	//a point on a face of the unit box, from s across the skin rect (left to right) and t down it
	//half the widest part, in blocks: how far a toppled mob's side sits from its centre line
	float size_of_flank(const Mob& mob) {
		float half = 0.0f;
		for (const ModelPart& part : mob.type.model.parts) {
			if (part.parent < 0) half = glm::max(half, glm::max(std::abs(part.from.x), std::abs(part.from.x + part.size.x)));
		}
		return half / 16.0f;
	}

	vec3 face_point(int face, float s, float t) {
		switch (face) {
		case 0: return vec3(1, 1 - t, s);
		case 1: return vec3(1 - s, 1 - t, 1);
		case 2: return vec3(0, 1 - t, 1 - s);
		case 3: return vec3(s, 1 - t, 0);
		case 4: return vec3(1 - s, 1, t);
		default: return vec3(1 - s, 0, t);
		}
	}
}

MobRenderer::MobRenderer() : sm(ShaderManager::get_instance()) {
	glGenBuffers(1, &matrix_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, matrix_buffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(vec4), nullptr, GL_STREAM_DRAW);
	glGenTextures(1, &matrix_texture);
	glBindTexture(GL_TEXTURE_BUFFER, matrix_texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, matrix_buffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

MobRenderer::~MobRenderer() {
	glDeleteTextures(1, &matrix_texture);
	glDeleteBuffers(1, &matrix_buffer);
}

MobRenderer::TypeMesh& MobRenderer::mesh_for(const MobType& type) {
	auto found = meshes.find(&type);
	if (found != meshes.end()) return *found->second;

	auto mesh = make_unique<TypeMesh>();
	const MobModel& model = type.model;
	vec2 skin_size = vec2(model.skin_size);
	vector<mob_vertex> vertices;
	const vec2 corners[6] = { vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(1, 1), vec2(0, 1), vec2(0, 0) };
	for (size_t p = 0; p < model.parts.size(); ++p) {
		const ModelPart& part = model.parts[p];
		for (int face = 0; face < 6; ++face) {
			vec4 rect = face_rect(face, part.size);
			for (vec2 c : corners) {
				vec3 local = (part.from + face_point(face, c.x, c.y) * part.size) / 16.0f;
				vec2 pixel = vec2(part.uv) + vec2(rect.x + c.x * rect.z, rect.y + c.y * rect.w);
				//skins load flipped, so the image's top row is v = 1
				vec2 uv = vec2(pixel.x / skin_size.x, 1.0f - pixel.y / skin_size.y);
				vertices.push_back({ local, face_normals[face], uv, (float)p });
			}
		}
	}
	mesh->vertex_count = (GLsizei)vertices.size();
	mesh->texels_per_mob = (int)model.parts.size() * 4 + 1;
	mesh->vbo.reset_vertices(vertices.data(), sizeof(mob_vertex) * vertices.size(), GL_STATIC_DRAW);
	mesh->vao.bind();
	mesh->vao.link_attrib(mesh->vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(mob_vertex), (void*)offsetof(mob_vertex, position));
	mesh->vao.link_attrib(mesh->vbo, 1, 3, GL_FLOAT, GL_FALSE, sizeof(mob_vertex), (void*)offsetof(mob_vertex, normal));
	mesh->vao.link_attrib(mesh->vbo, 2, 2, GL_FLOAT, GL_FALSE, sizeof(mob_vertex), (void*)offsetof(mob_vertex, uv));
	mesh->vao.link_attrib(mesh->vbo, 3, 1, GL_FLOAT, GL_FALSE, sizeof(mob_vertex), (void*)offsetof(mob_vertex, part));
	mesh->vao.unbind();
	mesh->skin = make_unique<Texture>(model.skin.c_str(), GL_TEXTURE0 + skin_unit, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);

	TypeMesh& result = *mesh;
	meshes[&type] = std::move(mesh);
	return result;
}

//a matrix per part, parents first, then the daylight at the mob and whether it flashes red
void MobRenderer::pose(const Mob& mob, vector<vec4>& out) const {
	const vector<ModelPart>& parts = mob.type.model.parts;
	mat4 root = mob.body_matrix();
	//dying topples onto its side, fast at first like a fall; a stranded fish just lies there
	float roll = 0.0f;
	if (mob.dying()) roll = sqrt(glm::min(1.0f, mob.death_time / 0.6f));
	else if (mob.stranded()) roll = 1.0f;
	if (roll > 0.0f) {
		//raised as it goes over, so the flank it lands on rests on the ground instead of in it
		root = rotate(translate(root, vec3(0.0f, roll * size_of_flank(mob), 0.0f)), roll * 1.5707963f, vec3(0, 0, 1));
	}
	float wave = sin(mob.walk_phase);
	float swing = wave * 0.5f * mob.leg_swing;
	float tail = wave * (0.25f + 0.3f * mob.leg_swing);
	float flap = wave * (0.2f + 0.4f * mob.leg_swing);
	//wings: swept back and folded on the ground, spread and beating in the air
	float spread = mob.wing_spread;
	float beat = sin(mob.flap_phase);
	float wing_sweep = 1.25f * (1.0f - spread);
	float wing_lift = mix(0.25f, 0.1f + beat * 0.75f, spread);
	float tip_sweep = 1.0f * (1.0f - spread);
	float tip_lift = spread * sin(mob.flap_phase - 0.9f) * 0.5f;

	vector<mat4> matrices(parts.size());
	for (size_t i = 0; i < parts.size(); ++i) {
		const ModelPart& part = parts[i];
		mat4 m = translate(part.parent < 0 ? root : matrices[part.parent], part.pivot / 16.0f);
		if (part.rest != vec3(0.0f)) {
			m = rotate(rotate(rotate(m, part.rest.z, vec3(0, 0, 1)), part.rest.y, vec3(0, 1, 0)), part.rest.x, vec3(1, 0, 0));
		}
		switch (part.motion) {
		case part_motion::head:
			m = rotate(rotate(m, mob.head_yaw, vec3(0, 1, 0)), mob.head_pitch, vec3(1, 0, 0));
			break;
		case part_motion::leg_forward:
			m = rotate(m, swing, vec3(1, 0, 0));
			break;
		case part_motion::leg_back:
			m = rotate(m, -swing, vec3(1, 0, 0));
			break;
		case part_motion::tail_sway:
			m = rotate(m, tail, vec3(0, 1, 0));
			break;
		case part_motion::tail_beat:
			m = rotate(m, tail * 0.7f, vec3(1, 0, 0));
			break;
		case part_motion::flipper_left:
			m = rotate(m, flap, vec3(0, 0, 1));
			break;
		case part_motion::flipper_right:
			m = rotate(m, -flap, vec3(0, 0, 1));
			break;
		case part_motion::wing_left:
			m = rotate(rotate(m, wing_sweep, vec3(0, 1, 0)), wing_lift, vec3(0, 0, 1));
			break;
		case part_motion::wing_right:
			m = rotate(rotate(m, -wing_sweep, vec3(0, 1, 0)), -wing_lift, vec3(0, 0, 1));
			break;
		case part_motion::wing_tip_left:
			m = rotate(rotate(m, tip_sweep, vec3(0, 1, 0)), tip_lift, vec3(0, 0, 1));
			break;
		case part_motion::wing_tip_right:
			m = rotate(rotate(m, -tip_sweep, vec3(0, 1, 0)), -tip_lift, vec3(0, 0, 1));
			break;
		case part_motion::serpent:
			//each link a little behind the one before, so the ripple travels down the body
			m = rotate(m, sin(mob.flap_phase - i * 0.55f) * 0.3f, vec3(0, 1, 0));
			m = rotate(m, sin(mob.flap_phase * 0.8f - i * 0.45f) * 0.1f, vec3(1, 0, 0));
			break;
		default:
			break;
		}
		matrices[i] = m;
		for (int c = 0; c < 4; ++c) out.push_back(m[c]);
	}

	float light = 1.0f;
	Chunk* chunk = ChunkManager::get_instance().get_chunk(mob.position);
	if (chunk != nullptr) {
		ivec3 local = world_to_local_coord(mob.position);
		light = daylight_level(chunk->get_height(local.x, local.z), (int)std::round(mob.position.y)) / 15.0f;
	}
	float flash = mob.hurt_time > 0.0f || mob.dying() ? 1.0f : 0.0f;
	out.push_back(vec4(light, flash, 0.0f, 0.0f));
}

void MobRenderer::build(const vector<unique_ptr<Mob>>& mobs, const vector<const Mob*>& extras) {
	for (auto& entry : meshes) entry.second->instances = 0;
	map<const MobType*, vector<const Mob*>> by_type;
	for (const auto& m : mobs) by_type[&m->type].push_back(m.get());
	for (const Mob* extra : extras) by_type[&extra->type].push_back(extra);

	texels.clear();
	for (auto& entry : by_type) {
		TypeMesh& mesh = mesh_for(*entry.first);
		mesh.first_texel = (int)texels.size();
		mesh.instances = (int)entry.second.size();
		for (const Mob* m : entry.second) pose(*m, texels);
	}
	if (texels.empty()) return;
	glBindBuffer(GL_TEXTURE_BUFFER, matrix_buffer);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(vec4) * texels.size(), texels.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

//culling stays off: the boxes are closed, so the depth test already hides their far sides
void MobRenderer::draw_all(Shader& shader, bool with_skin, bool overlay) {
	glActiveTexture(GL_TEXTURE0 + matrix_unit);
	glBindTexture(GL_TEXTURE_BUFFER, matrix_texture);
	glDisable(GL_CULL_FACE);
	for (auto& entry : meshes) {
		TypeMesh& mesh = *entry.second;
		if (mesh.instances == 0 || entry.first->overlay != overlay) continue;
		if (with_skin) {
			mesh.skin->activate();
			mesh.skin->bind();
		}
		shader.set_uniform_1i("first_texel", mesh.first_texel);
		shader.set_uniform_1i("texels_per_mob", mesh.texels_per_mob);
		mesh.vao.bind();
		glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.vertex_count, mesh.instances);
	}
	glEnable(GL_CULL_FACE);
	glActiveTexture(GL_TEXTURE0);
}

void MobRenderer::draw() {
	if (texels.empty()) return;
	sm.mob_shader.activate();
	draw_all(sm.mob_shader, true, false);
}

void MobRenderer::draw_overlay() {
	if (texels.empty()) return;
	sm.mob_shader.activate();
	draw_all(sm.mob_shader, true, true);
}

void MobRenderer::draw_depth(const mat4& light_space_matrix) {
	if (texels.empty()) return;
	sm.mob_shadow_shader.activate();
	sm.mob_shadow_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, light_space_matrix);
	draw_all(sm.mob_shadow_shader, false, false);
}
