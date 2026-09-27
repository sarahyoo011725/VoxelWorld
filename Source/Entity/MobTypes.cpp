#include "MobType.h"
#include "Goal.h"

namespace {
	const string sounds = "Resources/Sound Effects/";
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
		t.sounds.ambient = { sounds + "sheep_bleat.ogg", sounds + "sheep_bleat2.ogg" };
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
		t.sounds.hurt = { sounds + "cow_hurt.ogg" };
		t.sounds.death = { sounds + "cow_death.ogg" };
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
	const vector<biome_id> seas = { biome_id::ocean, biome_id::beach };

	//a fish: water only, a sideways tail, flops and suffocates on land
	MobType fish(const string& name, vec3 hitbox, float speed, vector<ModelPart> parts, SpawnRule spawn) {
		MobType t;
		t.name = name;
		t.hitbox = hitbox;
		t.lives = habitat::water;
		t.walk_speed = 0.0f;
		t.swim_speed = speed;
		t.turn_rate = 4.0f;
		t.max_health = 3.0f;
		t.dry_out_time = 2.0f;
		t.model.skin = "Resources/Textures/Mobs/" + name + ".png";
		t.model.parts = std::move(parts);
		t.spawn = std::move(spawn);
		t.goals = { goal<PanicGoal>(1), goal<SchoolGoal>(5), goal<SwimGoal>(5) };
		return t;
	}

	MobType cod() {
		SpawnRule spawn = { seas, 10, 3, 6 };
		spawn.min_depth = 3;
		return fish("cod", vec3(0.5f, 0.3f, 0.5f), 1.6f, {
			{ -1, vec3(0), vec3(-1, 0, -3), vec3(2, 4, 7), ivec2(0, 0) },
			{ -1, vec3(0), vec3(-1, 0.5f, 4), vec3(2, 3, 3), ivec2(0, 12) },
			{ -1, vec3(0, 2, -3), vec3(-0.5f, -2, -5), vec3(1, 4, 5), ivec2(12, 12), part_motion::tail_sway },
			{ -1, vec3(0), vec3(-0.5f, 4, -1), vec3(1, 1, 4), ivec2(24, 12) },
		}, spawn);
	}

	MobType salmon() {
		SpawnRule spawn = { { biome_id::ocean, biome_id::beach, biome_id::tundra, biome_id::taiga }, 6, 3, 5 };
		spawn.min_depth = 3;
		return fish("salmon", vec3(0.7f, 0.4f, 0.7f), 2.0f, {
			{ -1, vec3(0), vec3(-1.5f, 0, -4), vec3(3, 5, 8), ivec2(0, 0) },
			{ -1, vec3(0), vec3(-1.5f, 0.5f, 4), vec3(3, 4, 3), ivec2(0, 14) },
			{ -1, vec3(0, 2.5f, -4), vec3(-0.5f, -2.5f, -6), vec3(1, 5, 6), ivec2(14, 14), part_motion::tail_sway },
			{ -1, vec3(0), vec3(-0.5f, 5, -2), vec3(1, 2, 4), ivec2(30, 14) },
		}, spawn);
	}

	MobType tropical_fish() {
		SpawnRule spawn = { { biome_id::ocean }, 6, 3, 6 };
		spawn.min_depth = 3;
		return fish("tropical_fish", vec3(0.5f, 0.4f, 0.5f), 1.4f, {
			{ -1, vec3(0), vec3(-1, 0, -2.5f), vec3(2, 5, 5), ivec2(0, 0) },
			{ -1, vec3(0, 2.5f, -2.5f), vec3(-0.5f, -2, -3), vec3(1, 4, 3), ivec2(0, 11), part_motion::tail_sway },
			{ -1, vec3(0), vec3(-0.5f, 5, -2), vec3(1, 2, 4), ivec2(10, 11) },
			{ -1, vec3(0), vec3(-0.5f, -1, -1), vec3(1, 1, 3), ivec2(22, 11) },
		}, spawn);
	}

	MobType dolphin() {
		MobType t;
		t.name = "dolphin";
		t.hitbox = vec3(0.9f, 0.6f, 0.9f);
		t.lives = habitat::water;
		t.walk_speed = 0.0f;
		t.swim_speed = 4.0f;
		t.turn_rate = 2.5f;
		t.max_health = 10.0f;
		t.dry_out_time = 20.0f;
		t.model.skin = "Resources/Textures/Mobs/dolphin.png";
		t.model.parts = {
			{ -1, vec3(0), vec3(-4, 0, -6), vec3(8, 7, 12), ivec2(0, 0) },
			{ -1, vec3(0, 0, 6), vec3(-3.5f, 0, 0), vec3(7, 6, 6), ivec2(0, 20) },
			{ 1, vec3(0), vec3(-1.5f, 0, 6), vec3(3, 2, 4), ivec2(26, 20) },
			{ -1, vec3(0), vec3(-0.5f, 7, -1), vec3(1, 4, 3), ivec2(40, 20) },
			{ -1, vec3(0, 3.5f, -6), vec3(-2, -2, -8), vec3(4, 4, 8), ivec2(0, 33), part_motion::tail_beat },
			{ 4, vec3(0, 0, -8), vec3(-5, -0.5f, -4), vec3(10, 1, 4), ivec2(24, 33), part_motion::tail_beat },
			{ -1, vec3(4, 1, 3), vec3(0, -0.5f, -1.5f), vec3(4, 1, 3), ivec2(24, 39), part_motion::flipper_left },
			{ -1, vec3(-4, 1, 3), vec3(-4, -0.5f, -1.5f), vec3(4, 1, 3), ivec2(24, 39), part_motion::flipper_right },
		};
		t.spawn = { { biome_id::ocean }, 3, 2, 3 };
		t.spawn.min_depth = 5; //oceans here bottom out at 7 deep
		t.goals = { goal<PanicGoal>(1), goal<BreachGoal>(5), goal<SchoolGoal>(5, 12.0f), goal<SwimGoal>(5) };
		return t;
	}

	MobType seal() {
		MobType t;
		t.name = "seal";
		t.hitbox = vec3(0.9f, 0.6f, 0.9f);
		t.lives = habitat::amphibious;
		t.walk_speed = 0.5f; //a slow shuffle on land
		t.swim_speed = 2.5f;
		t.turn_rate = 2.0f;
		t.max_health = 10.0f;
		t.model.skin = "Resources/Textures/Mobs/seal.png";
		t.model.parts = {
			{ -1, vec3(0), vec3(-4, 0, -7), vec3(8, 6, 14), ivec2(0, 0) },
			{ -1, vec3(0, 4, 7), vec3(-3, -1, 0), vec3(6, 6, 5), ivec2(0, 21), part_motion::head },
			{ -1, vec3(4, 1, 4), vec3(0, -0.5f, -1.5f), vec3(4, 1, 3), ivec2(22, 21), part_motion::flipper_left },
			{ -1, vec3(-4, 1, 4), vec3(-4, -0.5f, -1.5f), vec3(4, 1, 3), ivec2(22, 21), part_motion::flipper_right },
			{ -1, vec3(1.5f, 1, -7), vec3(-1.5f, -0.5f, -5), vec3(3, 1, 5), ivec2(22, 26), part_motion::tail_beat },
			{ -1, vec3(-1.5f, 1, -7), vec3(-1.5f, -0.5f, -5), vec3(3, 1, 5), ivec2(22, 26), part_motion::tail_beat },
		};
		t.spawn = { { biome_id::beach, biome_id::ocean, biome_id::tundra }, 8, 1, 3 };
		t.spawn.ground = { sand, gravel, snow, stone };
		t.spawn.shore_distance = 4;
		t.goals = { goal<PanicGoal>(1), goal<HaulOutGoal>(5), goal<SwimGoal>(5), goal<WanderGoal>(5, 0.2f), goal<LookAtPlayerGoal>(5) };
		return t;
	}
}

const vector<MobType>& mob_types() {
	static const vector<MobType> types = { sheep(), cow(), pig(), seal(), dolphin(), cod(), salmon(), tropical_fish() };
	return types;
}
