#pragma once

#include "Vector4.h"

typedef Vector4 Quaternion;

struct Transform
{
	Vector4 Translation;
	Vector4 Scale;
	Quaternion Rotation;
};