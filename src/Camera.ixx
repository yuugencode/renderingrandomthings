module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

export module Camera;

import Transform;
import Utils;

export struct Camera {
	Transform transform;
	float fov;
	float nearClip, farClip;
};