#pragma once
#include "World/MobManager.h"
#include <string>

/*
	runs one line typed into the console, e.g. "/summon dragon", and returns what
	to tell the player. eye and look place anything summoned in front of them
*/
//teleport_to, if given, is set when the command moves the player
std::string run_command(const std::string& line, MobManager& mobs, vec3 eye, vec3 look, vec3* teleport_to = nullptr);
