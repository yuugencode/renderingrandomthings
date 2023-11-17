#pragma once

#include <glm/glm.hpp>

#include "Game/Entity.h"

// Raycast result
struct RayResult {
	glm::vec3 localPos;
	glm::vec3 faceNormal;
	Entity* obj;
	float depth;
	int data;
	bool Hit() const { return obj != nullptr; }
};

// Data passed along the recursion during raytracing
class TraceData {
public:
	enum Arg : int {
		Shadows = 1 << 0,
		Indirect = 1 << 1,
		Reflection = 1 << 2,
		Skybox = 1 << 3,
		Ambient = 1 << 4,
		Transparent = 1 << 5,
		Default = Shadows | Indirect | Reflection | Skybox | Ambient | Transparent,
	};
	constexpr TraceData() { };
	constexpr TraceData(int i) { val = (Arg)i; } // Allow logical or/and

	bool HasFlag(const Arg& arg) const { return (val & arg) != 0; }

	float cumulativeDepth = 0.0f;
	int recursionDepth = 0;

private:
	Arg val = Default;
};