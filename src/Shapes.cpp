#include "Shapes.h"

// Sphere

Sphere::Sphere(const glm::vec3& pos, const float& radius) {
	aabb = AABB(radius * 2.00001f); // Fixes some precision errors on indirect bounces
	transform.position = pos;
	transform.rotation = glm::quat();
	transform.scale = glm::vec3(radius, radius, radius);
	type = Entity::Type::Sphere;
	shaderType = Shader::Normals;
	id = idCount++;
}

// All the local intersection tests are against an object of size 1 in the middle due to inv transformed ray

bool Sphere::IntersectLocal(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const {
	if (ray.mask == id) return false;
	// Adapted from Inigo Quilez https://iquilezles.org/articles/
	auto nrm_rd = glm::normalize(ray.rd);
	float b = glm::dot(ray.ro, nrm_rd);
	if (b > 0.0f) return false; // Looking 180 degs away
	float c = glm::dot(ray.ro, ray.ro) - 1.0f;
	if (c < 0.0f) return false; // Ray inside sphere
	float h = b * b - c;
	if (h < 0.0f) return false;
	depth = (-b - sqrt(h)) * transform.scale.x;
	normal = glm::normalize(ray.ro + ray.rd * depth);
	data = id;
	return true;
}

glm::vec3 Sphere::LocalNormal(const glm::vec3& pos) const {
	return glm::normalize(pos - transform.position);
}

v2f Sphere::VertexShader(const glm::vec3& worldPos, const glm::vec3& localPos, const glm::vec3& localNormal, const uint32_t& data) const {
	return v2f{ .worldPosition = worldPos, .localPosition = localPos, .localNormal = localNormal, .uv = glm::vec2(0), .data = data };
}

// Disk

glm::vec3 Disk::LocalNormal() const { return transform.rotation * glm::vec3(0,1,0); }

Disk::Disk(const glm::vec3& pos, const glm::vec3& normal, const float& radius) {
	aabb = AABB(radius);
	transform.position = pos;
	transform.rotation = glm::normalize(glm::quat(1.0f, normal.x, normal.y, normal.z)); // Generate quat from a single direction
	transform.scale = glm::vec3(radius);
	type = Entity::Type::Disk;
	id = idCount++;
	shaderType = Shader::Grid;
}

bool Disk::IntersectLocal(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const {
	if (ray.mask == id) return false;
	glm::vec3 o = ray.ro;
	normal = LocalNormal();
	depth = -glm::dot(normal, o) / glm::dot(ray.rd, normal);
	data = id;
	if (depth < 0.0f) return false;
	glm::vec3  q = o + ray.rd * depth;
	if (dot(q, q) < 1.0f) return true;
	return false;
}

v2f Disk::VertexShader(const glm::vec3& worldPos, const glm::vec3& localPos, const glm::vec3& faceNormal, const uint32_t& data) const {
	return v2f{ .worldPosition = worldPos, .localPosition = localPos, .localNormal = faceNormal, .uv = glm::vec2(0), .data = data };
}

// Box

Box::Box(const glm::vec3& pos, const glm::vec3& size) {
	transform.position = pos;
	transform.scale = size;
	aabb = AABB(-size, size);
	type = Entity::Type::Box;
	id = idCount++;
	shaderType = Shader::PlainWhite;
}

bool Box::IntersectLocal(const Ray& ray, glm::vec3& normal, uint32_t& data, float& depth) const {
	if (ray.mask == id) return false;
	auto res = AABB(-transform.scale, transform.scale).Intersect(ray);
	if (res > 0.0f && id != ray.mask) {
		depth = res;
		normal = LocalNormal(ray.ro + ray.rd * res);
		data = id;
		return true;
	}
	return false;
}

glm::vec3 Box::LocalNormal(const glm::vec3& pos) const {
	const auto nrm = pos / transform.scale;
	if (glm::abs(nrm.x) > glm::abs(nrm.y) && glm::abs(nrm.x) > glm::abs(nrm.z))
		return nrm.x < 0.0f ? glm::vec3(-1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
	else if (glm::abs(nrm.y) > glm::abs(nrm.z))
		return nrm.y < 0.0f ? glm::vec3(0.0f, -1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	else
		return nrm.z < 0.0f ? glm::vec3(0.0f, 0.0f, -1.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
}

v2f Box::VertexShader(const glm::vec3& worldPos, const glm::vec3& localPos, const glm::vec3& localFaceNormal, const uint32_t& data) const {
	return v2f{ .worldPosition = worldPos, .localPosition = localPos, .localNormal = localFaceNormal, .uv = glm::vec2(0), .data = data };
}
