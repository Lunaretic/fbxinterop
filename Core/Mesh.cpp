#pragma once

#include "Transform.h"
#include "Vertex.h"
#include "Mesh.h"
#include <iostream>

Mesh::Mesh(int index, Vertex* vs, int numv, unsigned short* inds, int numi, unsigned short* iBoneList, int iBoneListSize)
{
	this->index = index;

	this->vertices = new Vertex[numv];
	this->indices = new unsigned short[numi];
	this->boneList = new unsigned short[iBoneListSize];
	
	memcpy(this->vertices, vs, numv * sizeof(Vertex));
	memcpy(this->indices, inds, numi * sizeof(unsigned short));
	memcpy(this->boneList, iBoneList, iBoneListSize * sizeof(unsigned short));

	this->numVertices = numv;
	this->numIndices = numi;
	this->boneListSize = iBoneListSize;
}

Mesh::~Mesh()
{
	delete[] vertices;
	delete[] indices;
	delete[] boneList;
}
