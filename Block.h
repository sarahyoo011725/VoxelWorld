#pragma once

#include <GLFW/glfw3.h>

enum BlockType {
	None,
	Dirt,
	Stone,
	Grass,
	Wood,
	Sand
};

class Block
{
public:
	bool active = true;
	BlockType type;
	Block(BlockType type);
};

//cube verticies
GLfloat cube_vertices[] = { 
//    x     y      z      texture
	//front
	-0.5, 0.5, 0.5,		0.0, 1.0,	// Front Top Left		
	0.5,  0.5, 0.5,		1.0, 1.0,	// Front Top Right	
	0.5, -0.5, 0.5,		1.0, 0.0,	// Front Bottom Right	
	-0.5,-0.5, 0.5,		0.0, 0.0,	// Front Bottom Left
	//back
	-0.5, 0.5,-0.5,		0.0, 1.0,	// Back Top Left
	0.5, 0.5, -0.5,		1.0, 1.0, 	// Back Top Right
	0.5,-0.5, -0.5,		1.0, 0.0,	// Back Bottom Right
	-0.5,-0.5,-0.5,		0.0, 0.0,	// Back Bottom Left		
	//left
	-0.5, 0.5, 0.5,		0.0, 1.0,
	-0.5,0.5, -0.5, 	1.0, 1.0,
	-0.5,-0.5,-0.5,		1.0, 0.0,
	-0.5, -0.5, 0.5,	0.0, 0.0,
	//right
	0.5, 0.5, 0.5,		0.0, 1.0,
	0.5, 0.5, -0.5,		1.0, 1.0,
	0.5,-0.5, -0.5,		1.0, 0.0,
	0.5, -0.5, 0.5,		0.0, 0.0,
	//top
	-0.5, 0.5, -0.5,	0.0, 1.0,
	0.5, 0.5, -0.5,		1.0, 1.0,
	0.5, 0.5, 0.5,		1.0, 0.0,
	-0.5, 0.5, 0.5,		0.0, 0.0,
	//bottom
	-0.5, -0.5, -0.5,	0.0, 1.0,
	0.5, -0.5, -0.5,	1.0, 1.0,
	0.5, -0.5, 0.5,		1.0, 0.0,
	-0.5, -0.5, 0.5,	0.0, 0.0,
};
//cube indices
GLuint cube_indices[] = {
//front
	0,1,2,  
	2,3,0,
//back
	4,5,6,	
	6,7,4,
//left
	8,9,10,
	10,11,8,
//right
	12,13,14,
	14,15,12,
//top
	16,17,18,
	18,19,16,
//bottom
	20,21,22,
	22,23,20,
};

