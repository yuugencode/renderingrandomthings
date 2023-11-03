#pragma once

#include "Transform.h"

// Standard camera with position, fov etc
struct Camera {
	Transform transform;
	float fov;
	float nearClip, farClip;
};
