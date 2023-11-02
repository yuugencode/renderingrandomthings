module;

#include <glm/glm.hpp>

module Raytracer;

import Utils;
import Scene;
import Entity;
import Shaders;
import Assets;
import Texture;

// Shaders depend on TraceRay + TraceRay depends on Shaders so this is a separate file, rip modules..

glm::vec4 GetColor(const Scene& scene, const Entity* obj, const glm::vec3& hitPt, const glm::vec3& normal, const uint32_t& data) {

	// Interpolate variables depending on position in "vertex shader"
	const v2f interpolated = obj->VertexShader(hitPt, normal, data);

	glm::vec4 c;

	// Shade below in "fragment shader"
	switch (obj->shaderType) {
		case Entity::Shader::PlainWhite:c = Shaders::PlainWhite(scene, obj, interpolated); break;
		case Entity::Shader::Textured:	c = Shaders::Textured(scene, obj, interpolated); break;
		case Entity::Shader::Normals:	c = Shaders::Normals(scene, obj, interpolated); break;
		case Entity::Shader::Grid:		c = Shaders::Grid(scene, obj, interpolated); break;
		default: c = Shaders::PlainWhite(scene, obj, interpolated); break;
	}

	return c;
}

RayResult Raytracer::TraceScene(const Scene& scene, const Ray& ray) const {

	RayResult result{ .depth = std::numeric_limits<float>::max(), .normal = glm::vec3(0,1,0), .data = (uint32_t)-1, .obj = nullptr};

	// Loop all objects, could use something more sophisticated but a simple loop gets us pretty far
	for (const auto& entity : scene.entities) {

		// Early rejection for missed rays
		if (entity->aabb.Intersect(ray) == 0.0f) continue;

		bool intersect;
		float depth;
		uint32_t data;
		glm::vec3 nrm;

		// Switching here is inelegant but seems significantly faster than a virtual function call
		switch (entity->type) {
			case Entity::Type::Sphere: intersect = ((Sphere*)entity.get())->Intersect(ray, nrm, data, depth); break;
			case Entity::Type::Disk: intersect = ((Disk*)entity.get())->Intersect(ray, nrm, data, depth); break;
			case Entity::Type::Box: intersect = ((Box*)entity.get())->Intersect(ray, nrm, data, depth); break;
			case Entity::Type::RenderedMesh: intersect = ((RenderedMesh*)entity.get())->Intersect(ray, nrm, data, depth); break;
			default: break;
		}

		// Check if hit something and it's the closest hit
		if (intersect && depth < result.depth) {
			result.depth = depth;
			result.data = data;
			result.normal = nrm;
			result.obj = entity.get();
		}
	}

	return result;
}

glm::vec4 Raytracer::TraceRay(const Scene& scene, const Ray& ray, int& recursionDepth) const {

	RayResult result = TraceScene(scene, ray);

	if (result.Hit()) {
		// Hit something, color it
		const auto hitPt = ray.ro + ray.rd * result.depth;

		// Own color
		//normal = glm::vec3(1, 0, 0);
		glm::vec4 ownColor = GetColor(scene, result.obj, hitPt, result.normal, result.data);

		// Transparent or cutout
		if (ownColor.a < 0.99f && recursionDepth < 1) {
			//const auto newRo = hitPt + rd; // Go past the blocker
			Ray newRay{ .ro = hitPt, .rd = ray.rd, .inv_rd = ray.inv_rd, .mask = result.data };
			glm::vec4 behind = TraceRay(scene, newRay, ++recursionDepth);
			ownColor = Utils::Lerp(ownColor, behind, 1.0f - ownColor.a);
		}
		
		// Indirect bounce
		if (result.obj->reflectivity != 0.0f && recursionDepth < 2) {
			const auto newRo = glm::vec3(hitPt + result.normal * 0.000001f);
			const auto newRd = glm::reflect(ray.rd, result.normal);
			Ray newRay{ .ro = newRo, .rd = newRd, .inv_rd = 1.0f / newRd, .mask = result.data };
			glm::vec4 reflColor = TraceRay(scene, newRay, ++recursionDepth);
			ownColor = Utils::Lerp(ownColor, reflColor, result.obj->reflectivity);
		}

		return ownColor;
	}
	else {
		// If we hit nothing, draw "Skybox"
		return glm::vec4(0.3f, 0.3f, 0.6f, 1.0f);
	}
}