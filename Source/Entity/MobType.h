#pragma once
#include <glm/glm.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "World/Biome.h"
#include "Block/BlockType.h"

using namespace std;
using namespace glm;

class Goal;

//how a part moves on its own, on top of following its parent
enum class part_motion {
	none,
	head,
	leg_forward, leg_back, //swing forward and back, in diagonal pairs
	tail_sway, //side to side, like a fish
	tail_beat, //up and down, like a dolphin's flukes
	flipper_left, flipper_right, //flap up and down, mirrored
	wing_left, wing_right, //spread and beat in flight, folded back along the body on the ground
	wing_tip_left, wing_tip_right, //the outer half of a wing, beating a little behind the inner half
	serpent, //one link of a long body: a wave runs down the chain of them, head to tail
};

//where a mob can move under its own power
enum class habitat {
	land, //walks; stays out of water
	water, //swims; stranded on land it flops about and dries out
	amphibious, //walks on land and swims in water
};

/*
	one box of a model, in pixels (1/16 of a block). the box hangs off a pivot that
	sits relative to its parent's pivot (or the feet, for a root part) and turns
	about it. the skin holds the box unwrapped at `uv`, Minecraft style:

		      [ top ][bottom]            d tall
		[ +x ][front][ -x ][back ]       h tall
		  d      w     d      w

	where w, h, d are the box's size along x, y, z and the front faces +z
*/
struct ModelPart {
	int parent = -1;
	vec3 pivot = vec3(0.0f);
	vec3 from = vec3(0.0f);
	vec3 size = vec3(0.0f);
	ivec2 uv = ivec2(0);
	part_motion motion = part_motion::none;
	vec3 rest = vec3(0.0f); //radians about x, y, z the part is turned by before it moves, e.g. a neck angled up
};

struct MobModel {
	string skin; //png path
	ivec2 skin_size = ivec2(64, 64);
	vector<ModelPart> parts;
};

//files to pick from at random; an empty list stays silent
struct MobSounds {
	vector<string> ambient;
	float ambient_interval = 12.0f; //seconds between calls, on average
	vector<string> hurt;
	vector<string> death;
};

struct SpawnRule {
	vector<biome_id> biomes;
	int weight = 10; //relative odds against other types that can spawn in the same place
	int min_group = 2;
	int max_group = 4;
	vector<block_type> ground = { dirt_grass }; //what a land or amphibious mob spawns standing on
	int min_depth = 2; //water deep enough for a water mob's school
	int shore_distance = -1; //a land mob only spawns within this many blocks of water; negative anywhere
};

/*
	everything that makes one kind of mob: a new animal is a new entry in
	MobTypes.cpp, not a new class. behaviour is a list of reusable goals, so only
	something no existing goal covers needs new code
*/
struct MobType {
	string name;
	vec3 hitbox = vec3(0.9f);
	float walk_speed = 1.0f; //blocks per second
	float turn_rate = 2.5f; //radians per second
	float max_health = 10.0f;
	habitat lives = habitat::land;
	float swim_speed = 0.0f; //blocks per second
	float dry_out_time = -1.0f; //seconds a water mob lasts out of water before it starts to suffocate; negative never

	bool flies = false; //takes to the air to follow its owner, or when ridden and told to climb
	bool hovers = false; //a flier that never lands: it drifts in the air when idle
	float fly_speed = 0.0f; //blocks per second

	bool rideable = false;
	vec3 seat = vec3(0.0f); //where the rider sits, in model pixels from the feet
	float ride_speed = 0.0f; //on the ground, blocks per second
	float jump_speed = 0.0f; //upward speed of a ridden jump
	bool pet = false; //never spawns in the wild; one summoned is the player's, follows them and never despawns
	MobModel model;
	MobSounds sounds;
	SpawnRule spawn;
	vector<function<unique_ptr<Goal>()>> goals;
};

const vector<MobType>& mob_types();
const MobType* find_mob_type(const string& name);
//the player as seen from outside, sitting astride a mount; drawn only in third person
const MobType& rider_type();
