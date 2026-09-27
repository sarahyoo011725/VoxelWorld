#pragma once
#include <glm/glm.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "World/Biome.h"

using namespace std;
using namespace glm;

class Goal;

//how a part moves on its own, on top of following its parent
enum class part_motion { none, head, leg_forward, leg_back };

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
	MobModel model;
	MobSounds sounds;
	SpawnRule spawn;
	vector<function<unique_ptr<Goal>()>> goals;
};

const vector<MobType>& mob_types();
