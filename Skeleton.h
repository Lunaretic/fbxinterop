#pragma once

class Skeleton
{
public:
	Skeleton(unsigned char* data, int length, short* connectBones);
	~Skeleton();

	unsigned char* data;
	int length;
	short* connectBones;
};