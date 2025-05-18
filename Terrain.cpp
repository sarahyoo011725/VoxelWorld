#include "Terrain.h"

Terrain::Terrain(vec3 *cam_pos) {
	player_pos = cam_pos;
}

Terrain::~Terrain() {

}

void Terrain::render() {
	//cout << player_pos->y << endl;
}