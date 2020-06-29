#pragma once

#include "Transform.h"
#include "Vertex.h"
#include <string>
#include <vector>

class Mesh
{
public:
	Mesh(int index, Vertex* vs, int numv, unsigned short* inds, int numi, char** boneNames, int numBones);
	~Mesh();
	
	int index;
	Vertex* vertices;
	unsigned short* indices;
	std::vector<std::string> boneList;
	int numVertices;
	int numIndices;
	int numBones;
};

