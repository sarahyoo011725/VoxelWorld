#include "Commands.h"
#include "World/Onsen.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace {
	string lower(string s) {
		transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)tolower(c); });
		return s;
	}

	string upper(string s) {
		transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)toupper(c); });
		return s;
	}

	string spaced(string name) {
		replace(name.begin(), name.end(), '_', ' ');
		return upper(name);
	}

	//friendlier names for the ones people will type most
	string resolve_alias(const string& name) {
		if (name == "haku" || name == "dragon" || name == "eastern" || name == "serpent") return "eastern_dragon";
		return name;
	}

	string mob_list() {
		string names;
		for (const MobType& t : mob_types()) names += (names.empty() ? "" : " ") + upper(t.name);
		return names;
	}

	//the top of the ground under a spot, or none within reach
	bool ground_under(vec3 spot, float& feet_y) {
		ChunkManager& cm = ChunkManager::get_instance();
		for (int y = (int)std::round(spot.y) + 3; y > (int)std::round(spot.y) - 24; --y) {
			Block* b = cm.get_block_worldspace(vec3(std::round(spot.x), y, std::round(spot.z)));
			if (b != nullptr && is_solid(b->type)) {
				feet_y = y + 0.51f;
				return true;
			}
		}
		return false;
	}

	string summon(const string& what, MobManager& mobs, vec3 eye, vec3 look) {
		const MobType* type = find_mob_type(resolve_alias(what));
		if (type == nullptr) return "UNKNOWN MOB " + upper(what) + ". TRY " + mob_list();

		vec3 ahead = vec3(look.x, 0.0f, look.z);
		ahead = length(ahead) > 0.01f ? normalize(ahead) : vec3(0.0f, 0.0f, 1.0f);
		//step out until there's room, a little further for big things
		for (float distance = 2.0f + type->hitbox.x; distance < 12.0f; distance += 1.0f) {
			vec3 spot = eye + ahead * distance;
			float feet_y = spot.y - 1.0f;
			if (!ground_under(spot, feet_y)) continue;
			if (type->hovers) feet_y += 2.0f;
			Mob* mob = mobs.add(*type, vec3(spot.x, feet_y, spot.z));
			if (mob == nullptr) continue;
			mob->owned = type->pet;
			mob->target_yaw = mob->yaw = atan2(-ahead.x, -ahead.z); //facing the player
			return "SUMMONED " + spaced(type->name) + (type->rideable ? ". RIGHT CLICK IT TO RIDE" : "");
		}
		return "NO ROOM TO SUMMON " + spaced(type->name) + " HERE";
	}
}

string run_command(const string& line, MobManager& mobs, vec3 eye, vec3 look, vec3* teleport_to) {
	string text = lower(line);
	if (!text.empty() && text[0] == '/') text = text.substr(1);
	istringstream words(text);
	vector<string> args;
	for (string word; words >> word;) args.push_back(word);
	if (args.empty()) return "";

	if (args[0] == "help") {
		return "/SUMMON <MOB>  /LOCATE ONSEN  /TP ONSEN  /TP X Y Z";
	}
	if (args[0] == "locate" || (args[0] == "tp" && args.size() == 2)) {
		if (args.size() < 2 || args[1] != "onsen") return "TRY /LOCATE ONSEN";
		OnsenSite site;
		if (!onsen::nearest(eye, 4000.0f, site)) return "NO ONSEN WITHIN 4000 BLOCKS";
		int away = (int)length(vec2(site.centre.x - eye.x, site.centre.z - eye.z));
		string where = to_string(site.centre.x) + " " + to_string(site.centre.y) + " " + to_string(site.centre.z);
		if (args[0] == "locate") return "NEAREST ONSEN AT " + where + " - " + to_string(away) + " BLOCKS AWAY";
		if (teleport_to == nullptr) return "CANNOT TELEPORT FROM HERE";
		//just outside the torii, looking in
		*teleport_to = vec3(site.centre) + vec3(0.5f, 2.0f, onsen::radius + 3.5f);
		return "TELEPORTED TO THE ONSEN AT " + where;
	}
	if (args[0] == "tp") {
		if (args.size() < 4 || teleport_to == nullptr) return "TRY /TP X Y Z OR /TP ONSEN";
		try {
			*teleport_to = vec3(stof(args[1]), stof(args[2]), stof(args[3]));
		}
		catch (...) {
			return "TRY /TP X Y Z OR /TP ONSEN";
		}
		return "TELEPORTED TO " + args[1] + " " + args[2] + " " + args[3];
	}
	if (args[0] == "summon") {
		if (args.size() < 2) return "SUMMON WHAT? " + mob_list();
		return summon(args[1], mobs, eye, look);
	}
	return "UNKNOWN COMMAND " + upper(args[0]) + ". TRY /HELP";
}
