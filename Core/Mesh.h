#pragma once

#include "Transform.h"
#include "Vertex.h"

class Mesh
{
public:
	Mesh(int index, Vertex* vs, int numv, unsigned short* inds, int numi, unsigned short* iBoneList, int iBoneListSize);
	~Mesh();
	
	int index;
	Vertex* vertices;
	unsigned short* indices;
	unsigned short* boneList;
	int numVertices;
	int numIndices;
	int boneListSize;
};

