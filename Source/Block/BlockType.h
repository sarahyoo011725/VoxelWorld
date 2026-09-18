#pragma once
#include <glm/glm.hpp>
#include <map>

using namespace std;
using namespace glm;

//names block types. explicitly one byte: there is one of these per voxel, so
//the default int width would cost 4x the memory for 12 values
enum block_type : unsigned char {
	none,
	dirt,
	dirt_grass,
	stone,
	sand,
	wood,
	leaf, //single tile, tinted per biome; kept fully opaque (no cutout) so it culls like a solid block
	water,
	glass,
	grass,
	snow,
	flower_red,
	flower_yellow,
	flower_purple,
	flower_white,
};

//names block faces
enum block_face {
	Front,
	Back,
	Left,
	Right,
	Top,
	Bottom,
};

namespace {
	//tiles in the block atlas. tile coords below are 1-based with row 1 at the
	//TOP of the image; the loader flips vertically, so v is measured from the
	//bottom and a tile's v range is (rows - row) / rows upward
	const static int texture_rows = 34;
	const static int textures_columns = 16;

	static vec2 grass_text_coord = vec2(8, 3);

	//plants are cross-plane geometry rather than cubes, so they sit outside
	//texture_map and name their tile here instead
	inline vec2 plant_texture_coord(block_type type) {
		switch (type) {
		case flower_red: return vec2(5, 23);
		case flower_yellow: return vec2(8, 21);
		case flower_purple: return vec2(1, 23);
		case flower_white: return vec2(4, 23);
		}
		return grass_text_coord;
	}

	//grass and leaf tiles are greyscale so a biome colour can be multiplied in
	//(see Biome.h); everything else is used at its authored colour
	inline bool is_biome_tinted(block_type type) {
		switch (type) {
		case dirt_grass:
		case grass: //greyscale; flowers are not, so they keep their own colour
		case leaf:
			return true;
		}
		return false;
	}

	//only the grass block's TOP face carries the greyscale grass texture - its
	//sides are ordinary dirt-with-fringe and must not be tinted
	inline bool is_tinted_face(block_type type, block_face face) {
		if (type == dirt_grass) return face == Top;
		return is_biome_tinted(type);
	}

	//collects texture index on the texture atlance for block types
	static map<block_type, map<block_face, vec2>> texture_map = {
		{dirt, {{Front,vec2(3,1)},{Back,vec2(3,1)},{Left,vec2(3,1)},{Right,vec2(3,1)},{Top,vec2(3,1)},{Bottom,vec2(3,1)}}},
		{dirt_grass, {{Front,vec2(4,1)},{Back,vec2(4,1)},{Left,vec2(4,1)},{Right,vec2(4,1)},{Top,vec2(1,1)},{Bottom,vec2(3,1)}}},
		{stone, {{Front,vec2(2,1)},{Back,vec2(2,1)},{Left,vec2(2,1)},{Right,vec2(2,1)},{Top,vec2(2,1)},{Bottom,vec2(2,1)}}},
		{sand, {{Front,vec2(3,2)},{Back,vec2(3,2)},{Left,vec2(3,2)},{Right,vec2(3,2)},{Top,vec2(3,2)},{Bottom,vec2(3,2)}}},
		{snow, {{Front,vec2(3,5)},{Back,vec2(3,5)},{Left,vec2(3,5)},{Right,vec2(3,5)},{Top,vec2(3,5)},{Bottom,vec2(3,5)}}},
		{wood, {{Front,vec2(5,2)},{Back,vec2(5,2)},{Left,vec2(5,2)},{Right,vec2(5,2)},{Top,vec2(6,2)},{Bottom,vec2(6,2)}}},
		{leaf, {{Front,vec2(6,4)},{Back,vec2(6,4)},{Left,vec2(6,4)},{Right,vec2(6,4)},{Top,vec2(6,4)},{Bottom,vec2(6,4)}}},
		{water, {{Front,vec2(16,14)},{Back,vec2(16,14)},{Left,vec2(16,14)},{Right,vec2(16,14)},{Top,vec2(16,14)},{Bottom,vec2(16,14)}}},
		{glass, {{Front,vec2(2,4)},{Back,vec2(2,4)},{Left,vec2(2,4)},{Right,vec2(2,4)},{Top,vec2(2,4)},{Bottom,vec2(2,4)}}},
	};
}