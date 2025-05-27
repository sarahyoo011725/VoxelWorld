#pragma once

#include <glm/glm.hpp>
#include <map>

using namespace std;
using namespace glm;

enum block_type {
	none,
	dirt,
	grass,
	stone,
	sand,
	trunk,
	leaf_transp,
	leaf_red,
	leaf_yellow,
	water,
	glass,
};

enum block_face {
	Front,
	Back,
	Left,
	Right,
	Top,
	Bottom,
};

namespace {
	const static int texture_rows = 16;
	const static int textures_columns = 16;

	static bool has_transparency(block_type type) {
		switch (type) {
		case water:
		case glass:
		case leaf_transp:
			return true;
		}
		return false;
	}

	static map<block_type, map<block_face, vec2>> texture_map = {
		{dirt, {
			{Front,  vec2(3, 16)},
			{Back,   vec2(3, 16)},
			{Left,   vec2(3, 16)},
			{Right,	 vec2(3, 16)},
			{Top,	 vec2(3, 16)},
			{Bottom, vec2(3, 16)},
		}},
		{grass, {
			{Front,  vec2(4, 16)},
			{Back,   vec2(4, 16)},
			{Left,   vec2(4, 16)},
			{Right,	 vec2(4, 16)},
			{Top,	 vec2(1, 16)},
			{Bottom, vec2(3, 16)},
		}},
		{stone, {
			{Front,  vec2(2, 16)},
			{Back,   vec2(2, 16)},
			{Left,   vec2(2, 16)},
			{Right,	 vec2(2, 16)},
			{Top,	 vec2(2, 16)},
			{Bottom, vec2(2, 16)},
		}},
		{sand, {
			{Front,  vec2(3, 15)},
			{Back,   vec2(3, 15)},
			{Left,   vec2(3, 15)},
			{Right,	 vec2(3, 15)},
			{Top,	 vec2(3, 15)},
			{Bottom, vec2(3, 15)},
		}},
		{trunk, {
			{Front,  vec2(5, 15)},
			{Back,   vec2(5, 15)},
			{Left,   vec2(5, 15)},
			{Right,	 vec2(5, 15)},
			{Top,	 vec2(6, 15)},
			{Bottom, vec2(6, 15)},
		}},
		{leaf_transp, {
			{Front,  vec2(5, 13)},
			{Back,   vec2(5, 13)},
			{Left,   vec2(5, 13)},
			{Right,	 vec2(5, 13)},
			{Top,	 vec2(5, 13)},
			{Bottom, vec2(5, 13)},
		}},
		{leaf_red, {
			{Front,  vec2(6, 13)},
			{Back,   vec2(6, 13)},
			{Left,   vec2(6, 13)},
			{Right,	 vec2(6, 13)},
			{Top,	 vec2(6, 13)},
			{Bottom, vec2(6, 13)},
		}},
		{leaf_yellow, {
			{Front,  vec2(7, 13)},
			{Back,   vec2(7, 13)},
			{Left,   vec2(7, 13)},
			{Right,	 vec2(7, 13)},
			{Top,	 vec2(7, 13)},
			{Bottom, vec2(7, 13)},
		}},
		{water, {
			{Front,  vec2(16, 3)},
			{Back,   vec2(16, 3)},
			{Left,   vec2(16, 3)},
			{Right,	 vec2(16, 3)},
			{Top,	 vec2(16, 3)},
			{Bottom, vec2(16, 3)},
		}},
		{glass, {
			{Front,  vec2(2, 13)},
			{Back,   vec2(2, 13)},
			{Left,   vec2(2, 13)},
			{Right,	 vec2(2, 13)},
			{Top,	 vec2(2, 13)},
			{Bottom, vec2(2, 13)},
		}}
	};
}