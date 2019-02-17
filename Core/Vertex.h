#pragma once

#include "Vector4.h"

struct Vertex
{
	Vector4 Position;
	Vector4 BlendWeights;
	unsigned int BlendIndices;
	Vector4 Normal;

	Vector4 UV;
	Vector4 Color;
	Vector4 Tangent1;
	Vector4 Tangent2;
};