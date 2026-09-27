#include "MobType.h"
#include "Goal.h"

namespace {
	const vector<biome_id> grassy = { biome_id::plains, biome_id::savanna, biome_id::forest, biome_id::autumn_forest, biome_id::taiga };

	//four legs of the given size under a body: diagonal pairs swing together
	void add_legs(vector<ModelPart>& parts, vec3 leg_size, float hip_height, float half_width, float half_length) {
		const float sides[4][2] = { { 1, 1 }, { -1, -1 }, { -1, 1 }, { 1, -1 } };
		for (int i = 0; i < 4; ++i) {
			ModelPart leg;
			leg.pivot = vec3(sides[i][0] * half_width, hip_height, sides[i][1] * half_length);
			leg.from = vec3(-leg_size.x * 0.5f, -leg_size.y, -leg_size.z * 0.5f);
			leg.size = leg_size;
			leg.uv = ivec2(0, 32);
			leg.motion = i < 2 ? part_motion::leg_forward : part_motion::leg_back;
			parts.push_back(leg);
		}
	}

	template <class T, class... Args>
	function<unique_ptr<Goal>()> goal(Args... args) {
		return [=] { return make_unique<T>(args...); };
	}

	MobType sheep() {
		MobType t;
		t.name = "sheep";
		t.hitbox = vec3(0.9f, 1.3f, 0.9f);
		t.walk_speed = 1.1f;
		t.max_health = 8.0f;
		t.model.skin = "Resources/Textures/Mobs/sheep.png";
		t.model.parts = {
			{ -1, vec3(0, 9, 0), vec3(-5, 0, -8), vec3(10, 9, 16), ivec2(0, 0) },
			{ -1, vec3(0, 15, 7), vec3(-3, -1, 0), vec3(6, 6, 7), ivec2(16, 32), part_motion::head },
		};
		add_legs(t.model.parts, vec3(4, 9, 4), 9, 3, 5);
		t.spawn = { grassy, 12, 2, 4 };
		t.goals = { goal<PanicGoal>(1), goal<WanderGoal>(5), goal<GrazeGoal>(5), goal<LookAtPlayerGoal>(5) };
		return t;
	}

	MobType cow() {
		MobType t;
		t.name = "cow";
		t.hitbox = vec3(0.9f, 1.4f, 0.9f);
		t.walk_speed = 1.0f;
		t.turn_rate = 2.0f;
		t.model.skin = "Resources/Textures/Mobs/cow.png";
		t.model.parts = {
			{ -1, vec3(0, 12, 0), vec3(-6, 0, -9), vec3(12, 10, 18), ivec2(0, 0) },
			{ -1, vec3(0, 19, 9), vec3(-4, -4, 0), vec3(8, 8, 6), ivec2(16, 32), part_motion::head },
			{ 1, vec3(0), vec3(4, 3, 2), vec3(1, 3, 1), ivec2(44, 32) },
			{ 1, vec3(0), vec3(-5, 3, 2), vec3(1, 3, 1), ivec2(44, 32) },
		};
		add_legs(t.model.parts, vec3(4, 12, 4), 12, 4, 6);
		t.spawn = { grassy, 8, 2, 4 };
		t.goals = { goal<PanicGoal>(1), goal<WanderGoal>(5), goal<GrazeGoal>(5), goal<LookAtPlayerGoal>(5) };
		return t;
	}

	MobType pig() {
		MobType t;
		t.name = "pig";
		t.hitbox = vec3(0.9f, 0.9f, 0.9f);
		t.walk_speed = 1.2f;
		t.turn_rate = 3.0f;
		t.model.skin = "Resources/Textures/Mobs/pig.png";
		t.model.parts = {
			{ -1, vec3(0, 6, 0), vec3(-5, 0, -8), vec3(10, 8, 16), ivec2(0, 0) },
			{ -1, vec3(0, 11, 8), vec3(-4, -4, 0), vec3(8, 8, 8), ivec2(16, 32), part_motion::head },
			{ 1, vec3(0), vec3(-2, -3, 8), vec3(4, 3, 1), ivec2(48, 32) },
		};
		add_legs(t.model.parts, vec3(4, 6, 4), 6, 3, 5);
		t.spawn = { grassy, 10, 2, 4 };
		t.goals = { goal<PanicGoal>(1), goal<WanderGoal>(5), goal<LookAtPlayerGoal>(5) };
		return t;
	}
}

const vector<MobType>& mob_types() {
	static const vector<MobType> types = { sheep(), cow(), pig() };
	return types;
}
