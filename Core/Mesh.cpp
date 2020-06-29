#pragma once

#include "Transform.h"
#include "Vertex.h"
#include "Mesh.h"
#include <iostream>

Mesh::Mesh(int index, Vertex* vs, int numv, unsigned short* inds, int numi, char** boneNames, int numBones)
{
	this->index = index;

	this->vertices = new Vertex[numv];
	this->indices = new unsigned short[numi];
	//this->boneList = new unsigned short[iBoneListSize];
	this->boneList = std::vector<std::string>(numBones);
	
	memcpy(this->vertices, vs, numv * sizeof(Vertex));
	memcpy(this->indices, inds, numi * sizeof(unsigned short));

	for (int i = 0; i < numBones; i++)
		boneList[i] = boneNames[i];
	//memcpy(this->boneList, iBoneList, iBoneListSize * sizeof(unsigned short));

	this->numVertices = numv;
	this->numIndices = numi;
	this->numBones = numBones;
}

Mesh::~Mesh()
{
	delete[] vertices;
	delete[] indices;
}
