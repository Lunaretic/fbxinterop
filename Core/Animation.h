#pragma once

struct AnimationS
{
	int count;
	int length;
	unsigned char* data;
	const char** names;
};

class Animation
{
public:
	Animation(int count, int length, unsigned char* data, char** names);
	~Animation();

	int count;
	int length;
	unsigned char* data;
	char** names;
};

