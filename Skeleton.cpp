
#include "Skeleton.h"
#include <iostream>

Skeleton::Skeleton(unsigned char* data, int length, short* connectBones)
{
	this->data = new unsigned char[length];
	memcpy_s(this->data, length, data, length);

	this->length = length;

	this->connectBones = new short[4];
	memcpy_s(this->connectBones, 4, connectBones, 4);
}

Skeleton::~Skeleton()
{
	delete[] data;
	delete[] connectBones;
}
