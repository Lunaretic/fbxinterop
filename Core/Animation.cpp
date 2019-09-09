#pragma once

#include "Animation.h"
#include <iostream>

Animation::Animation(int count, int length, unsigned char* data, char** names)
{
	this->count = count;
	this->length = length;
	this->data = new unsigned char[length];
	memcpy_s(this->data, length, data, length);

	this->names = new char* [count];

	for (int i = 0; i < count; i++)
	{
		this->names[i] = new char[strlen(names[i]) * sizeof(char) + 1];
		strncpy_s(this->names[i], strlen(names[i]) * sizeof(char) + 1, names[i], _TRUNCATE);
	}
}

Animation::~Animation()
{
	delete[] data;
	delete[] names;
}
