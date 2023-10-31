module;

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

export module RenderedMesh;

import <memory>;
import Entity;
import Mesh;
import Bvh;
import Utils;
import Texture;
import Assets;

/// <summary> A mesh consisting of triangles that can be raytraced </summary>
export struct RenderedMesh : Entity {
public:

	RenderedMesh(const int& meshHandle) {
		this->type = Entity::Type::RenderedMesh;
		this->meshHandle = meshHandle;
	}

	void GenerateBVH() {
		// Construct a bvh
		Timer t(1);
		t.Start();
		bvh.Generate(GetMesh()->vertices, GetMesh()->triangles);
		aabb = bvh.stack[0].aabb;
		t.End();
		Log::Line("bvh gen", Log::FormatFloat((float)t.GetAveragedTime() * 1000.0f), "ms");
	}

	bool Intersect(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& invDir, float& depth, uint32_t& triIdx) const {
		return bvh.Intersect(ro, rd, invDir, depth, triIdx);
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::length(pos); // @TODO
	}

	glm::vec3 Barycentric(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) const {
		const auto& v0 = b - a, v1 = c - a, v2 = p - a;
		const float d00 = glm::dot(v0, v0);
		const float d01 = glm::dot(v0, v1);
		const float d11 = glm::dot(v1, v1);
		const float d20 = glm::dot(v2, v0);
		const float d21 = glm::dot(v2, v1);
		const float denom = d00 * d11 - d01 * d01;
		const float v = (d11 * d20 - d01 * d21) / denom;
		const float w = (d00 * d21 - d01 * d20) / denom;
		const float u = 1.0f - v - w;
		return glm::vec3(u, v, w);
	}

	Color GetColor(const glm::vec3& pos, const uint32_t& triIndex) const {
		// Samples the color of this rendered mesh at pos and tri index
		
		const auto& mesh = Assets::Meshes[meshHandle];
		const auto& v0i = mesh->triangles[triIndex + 0];
		const auto& v1i = mesh->triangles[triIndex + 1];
		const auto& v2i = mesh->triangles[triIndex + 2];
		const auto& p0 = mesh->vertices[v0i];
		const auto& p1 = mesh->vertices[v1i];
		const auto& p2 = mesh->vertices[v2i];
		const auto& c0 = mesh->colors[v0i];
		const auto& c1 = mesh->colors[v1i];
		const auto& c2 = mesh->colors[v2i];
		const auto& n0 = mesh->normals[v0i];
		const auto& n1 = mesh->normals[v1i];
		const auto& n2 = mesh->normals[v2i];
		const auto& uv0 = mesh->uvs[v0i];
		const auto& uv1 = mesh->uvs[v1i];
		const auto& uv2 = mesh->uvs[v2i];
		const auto& material = mesh->materials[triIndex / 3];

		// Barycentric interpolation as if we were a real shader
		const auto b = Barycentric(pos, p0, p1, p2);

		// Texture sample
		if (HasTexture()) {
			const auto uv = uv0 * b.x + uv1 * b.y + uv2 * b.z;

			const auto& texID = textureHandles[material];
			return Assets::Textures[texID]->SampleUVClamp(uv);
		}

		// Vertex normals
		if (HasMesh() && Assets::Meshes[meshHandle]->hasNormals) {
			auto ret = n0 * b.x + n1 * b.y + n2 * b.z;
			return Color::FromVec(ret, 1.0f);
		}
		
		// Face normals
		auto ret = glm::normalize(glm::cross(p0 - p1, p0 - p2));
		return Color::FromVec(ret, 1.0f);
	};
};