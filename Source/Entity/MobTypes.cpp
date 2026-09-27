#include "MobType.h"
#include "Goal.h"

namespace {
	const string sounds = "Resources/Sound Effects/";
	const vector<biome_id> grassy = { biome_id::plains, biome_id::savanna, biome_id::forest, biome_id::autumn_forest, biome_id::taiga };

	//four legs of the given size under a body: diagonal pairs swing together
	void add_legs(vector<ModelPart>& parts, vec3 leg_size, float hip_height, float half_width, float half_length, ivec2 uv = ivec2(0, 32)) {
		const float sides[4][2] = { { 1, 1 }, { -1, -1 }, { -1, 1 }, { 1, -1 } };
		for (int i = 0; i < 4; ++i) {
			ModelPart leg;
			leg.pivot = vec3(sides[i][0] * half_width, hip_height, sides[i][1] * half_length);
			leg.from = vec3(-leg_size.x * 0.5f, -leg_size.y, -leg_size.z * 0.5f);
			leg.size = leg_size;
			leg.uv = uv;
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
	const vector<biome_id> seas = { biome_id::ocean, biome_id::beach, biome_id::frozen_ocean };

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
		SpawnRule spawn = { { biome_id::ocean, biome_id::beach, biome_id::tundra, biome_id::taiga, biome_id::frozen_ocean }, 6, 3, 5 };
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

	//a seal of either coat: grey on temperate shores, white on snowy ones
	MobType seal(const string& name, vector<biome_id> coasts) {
		MobType t;
		t.name = name;
		t.hitbox = vec3(0.9f, 0.6f, 0.9f);
		t.lives = habitat::amphibious;
		t.walk_speed = 0.5f; //a slow shuffle on land
		t.swim_speed = 2.5f;
		t.turn_rate = 2.0f;
		t.max_health = 10.0f;
		t.model.skin = "Resources/Textures/Mobs/" + name + ".png";
		//a chubby pup: short round body, a big head held up high, little flippers
		t.model.parts = {
			{ -1, vec3(0), vec3(-4.5f, 0, -6), vec3(9, 7, 12), ivec2(0, 0) },
			{ -1, vec3(0, 4, 6), vec3(-4, -1.5f, 0), vec3(8, 7, 7), ivec2(0, 20), part_motion::head },
			{ -1, vec3(4.5f, 1, 3), vec3(0, -0.5f, -1.5f), vec3(3, 1, 3), ivec2(32, 20), part_motion::flipper_left },
			{ -1, vec3(-4.5f, 1, 3), vec3(-3, -0.5f, -1.5f), vec3(3, 1, 3), ivec2(32, 20), part_motion::flipper_right },
			{ -1, vec3(1.5f, 1, -6), vec3(-1.5f, -0.5f, -4), vec3(3, 1, 4), ivec2(32, 25), part_motion::tail_beat },
			{ -1, vec3(-1.5f, 1, -6), vec3(-1.5f, -0.5f, -4), vec3(3, 1, 4), ivec2(32, 25), part_motion::tail_beat },
		};
		t.spawn = { std::move(coasts), 8, 1, 3 };
		t.spawn.ground = { sand, gravel, snow, stone, ice };
		t.spawn.shore_distance = 4;
		t.goals = { goal<PanicGoal>(1), goal<HaulOutGoal>(5), goal<SwimGoal>(5), goal<WanderGoal>(5, 0.2f), goal<LookAtPlayerGoal>(5) };
		return t;
	}

	MobType horse() {
		MobType t;
		t.name = "horse";
		t.hitbox = vec3(1.3f, 1.6f, 1.3f);
		t.walk_speed = 1.8f;
		t.turn_rate = 2.0f;
		t.max_health = 20.0f;
		t.model.skin = "Resources/Textures/Mobs/horse.png";
		t.model.parts = {
			{ -1, vec3(0, 11, 0), vec3(-5, 0, -10), vec3(10, 9, 20), ivec2(0, 0) },
			//the neck carries the head, so grazing and looking about swing the whole neck
			{ -1, vec3(0, 17, 8), vec3(-2, 0, -2), vec3(4, 9, 5), ivec2(16, 32), part_motion::head },
			{ 1, vec3(0, 9, 0), vec3(-2.5f, -3, -1), vec3(5, 5, 9), ivec2(34, 32) },
			{ 2, vec3(0), vec3(-2, 2, 0), vec3(1, 2, 1), ivec2(20, 48) },
			{ 2, vec3(0), vec3(1, 2, 0), vec3(1, 2, 1), ivec2(20, 48) },
			{ 1, vec3(0), vec3(-1, 0, -3), vec3(2, 10, 1), ivec2(0, 48) },
			{ -1, vec3(0, 19, -10), vec3(-1.5f, -10, -2), vec3(3, 10, 3), ivec2(8, 48), part_motion::tail_sway },
		};
		add_legs(t.model.parts, vec3(4, 11, 4), 11, 3, 7);
		t.spawn = { { biome_id::plains, biome_id::savanna }, 5, 2, 4 };
		t.rideable = true;
		t.seat = vec3(0, 20, -1);
		t.ride_speed = 8.0f;
		t.jump_speed = 9.0f;
		t.goals = { goal<PanicGoal>(1), goal<WanderGoal>(5), goal<GrazeGoal>(5), goal<LookAtPlayerGoal>(5) };
		return t;
	}

	//a long wingless river dragon that swims through the air, after Haku
	MobType eastern_dragon() {
		MobType t;
		t.name = "eastern_dragon";
		t.hitbox = vec3(1.0f, 0.8f, 1.0f);
		t.walk_speed = 0.0f;
		t.turn_rate = 2.5f;
		t.max_health = 50.0f;
		t.flies = true;
		t.hovers = true;
		t.fly_speed = 12.0f;
		t.rideable = true;
		t.seat = vec3(0, 11, -9);
		t.pet = true;
		t.model.skin = "Resources/Textures/Mobs/eastern_dragon.png";
		t.model.skin_size = ivec2(128, 128);
		vector<ModelPart>& p = t.model.parts;
		p = {
			{ -1, vec3(0, 8, 0), vec3(-3, -3, -6), vec3(6, 6, 6), ivec2(0, 0) },
			//1: a long wolfish head with a snout, a jaw, swept horns, whiskers and a mane
			{ 0, vec3(0), vec3(-3.5f, -3, 0), vec3(7, 6, 9), ivec2(24, 0), part_motion::head },
			{ 1, vec3(0), vec3(-2.5f, -3, 9), vec3(5, 3, 5), ivec2(56, 0) },
			{ 1, vec3(0, -3, 6), vec3(-2.5f, -1, 0), vec3(5, 1, 6), ivec2(56, 8) },
			{ 1, vec3(2, 3, 2), vec3(-0.5f, 0, -0.5f), vec3(1, 4, 1), ivec2(78, 0), part_motion::none, vec3(-0.7f, 0, -0.25f) },
			{ 1, vec3(-2, 3, 2), vec3(-0.5f, 0, -0.5f), vec3(1, 4, 1), ivec2(78, 0), part_motion::none, vec3(-0.7f, 0, 0.25f) },
			{ 2, vec3(2.5f, -1, 4), vec3(-0.5f, -0.5f, 0), vec3(1, 1, 10), ivec2(82, 0), part_motion::none, vec3(0.2f, 2.5f, 0) },
			{ 2, vec3(-2.5f, -1, 4), vec3(-0.5f, -0.5f, 0), vec3(1, 1, 10), ivec2(82, 0), part_motion::none, vec3(0.2f, -2.5f, 0) },
			{ 1, vec3(0, 3, 0), vec3(-2.5f, 0, -4), vec3(5, 3, 8), ivec2(0, 15) },
		};
		//9: fifteen more links behind the first; the ripple runs down them from the head
		int previous = 0;
		vector<int> links = { 0 };
		for (int i = 1; i < 16; ++i) {
			p.push_back({ previous, vec3(0, 0, -6), vec3(-3, -3, -6), vec3(6, 6, 6), ivec2(0, 0), part_motion::serpent });
			previous = (int)p.size() - 1;
			links.push_back(previous);
		}
		for (int i = 1; i < 15; i += 2) {
			p.push_back({ links[i], vec3(0, 3, 0), vec3(-1, 0, -5), vec3(2, 3, 5), ivec2(26, 15) });
		}
		for (int pair : { 2, 11 }) {
			for (float side : { 1.0f, -1.0f }) {
				p.push_back({ links[pair], vec3(3 * side, -2, -3), vec3(-1, -5, -1), vec3(2, 5, 2), ivec2(40, 15),
					side > 0.0f ? part_motion::leg_forward : part_motion::leg_back, vec3(0.5f, 0, 0) });
			}
		}
		p.push_back({ links.back(), vec3(0, 0, -6), vec3(-0.5f, -2.5f, -6), vec3(1, 5, 6), ivec2(48, 15), part_motion::tail_sway });
		t.goals = { goal<FollowOwnerGoal>(2), goal<DriftGoal>(5), goal<LookAtPlayerGoal>(5, 12.0f) };
		return t;
	}

	MobType rider() {
		MobType t;
		t.name = "rider";
		t.hitbox = vec3(0.6f, 1.8f, 0.6f);
		t.model.skin = "Resources/Textures/Mobs/rider.png";
		t.model.parts = {
			{ -1, vec3(0), vec3(-4, 0, -2), vec3(8, 12, 4), ivec2(16, 16) },
			{ 0, vec3(0, 12, 0), vec3(-4, 0, -4), vec3(8, 8, 8), ivec2(0, 0), part_motion::head },
			//arms reaching forward to the reins, legs astride the mount
			{ 0, vec3(5, 11, 0), vec3(-1, -11, -2), vec3(3, 12, 4), ivec2(40, 16), part_motion::none, vec3(-0.9f, 0, 0.1f) },
			{ 0, vec3(-5, 11, 0), vec3(-2, -11, -2), vec3(3, 12, 4), ivec2(40, 16), part_motion::none, vec3(-0.9f, 0, -0.1f) },
			{ 0, vec3(2, 0, 0), vec3(-2, -12, -2), vec3(4, 12, 4), ivec2(0, 16), part_motion::none, vec3(-0.5f, 0, 0.45f) },
			{ 0, vec3(-2, 0, 0), vec3(-2, -12, -2), vec3(4, 12, 4), ivec2(0, 16), part_motion::none, vec3(-0.5f, 0, -0.45f) },
		};
		return t;
	}
}

const vector<MobType>& mob_types() {
	static const vector<MobType> types = { sheep(), cow(), pig(), horse(), seal("seal", { biome_id::beach, biome_id::ocean }),
		seal("snow_seal", { biome_id::tundra, biome_id::frozen_ocean }), dolphin(), cod(), salmon(), tropical_fish(),
		eastern_dragon() };
	return types;
}

const MobType* find_mob_type(const string& name) {
	for (const MobType& t : mob_types()) {
		if (t.name == name) return &t;
	}
	return nullptr;
}

const MobType& rider_type() {
	static const MobType type = rider();
	return type;
}
