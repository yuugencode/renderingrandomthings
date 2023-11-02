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
import Light;

/// <summary> A mesh consisting of triangles that can be raytraced </summary>
export struct RenderedMesh : Entity {
public:

	RenderedMesh(const int& meshHandle) {
		this->type = Entity::Type::RenderedMesh;
		this->meshHandle = meshHandle;
		id = idCount++;
		shaderType = Shader::Textured;
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

	bool Intersect(const Ray& ray, glm::vec3& normal, uint32_t& triIdx, float& depth) const {
		return bvh.Intersect(ray, normal, triIdx, depth);
	}

	float EstimatedDistanceTo(const glm::vec3& pos) const {
		return glm::length(pos - transform.position);
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

	v2f VertexShader(const glm::vec3& pos, const glm::vec3& faceNormal, const uint32_t& data) const {
		
		using namespace glm; // Pretend we're a shader
		
		v2f ret;

		const auto& mesh = Assets::Meshes[meshHandle];
		const auto& v0i = mesh->triangles[data + 0]; // data == triIndex for meshes
		const auto& v1i = mesh->triangles[data + 1];
		const auto& v2i = mesh->triangles[data + 2];
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
		const auto& material = mesh->materials[data / 3];

		// Barycentric interpolation
		const auto b = Barycentric(pos, p0, p1, p2);

		//vec4 diffuse(0, 0, 0, 1);
		//vec3 normal = faceNormal;

		ret.position = pos;
		ret.data = data;

		// Texture sample
		if (HasTexture()) {
			ret.uv = uv0 * b.x + uv1 * b.y + uv2 * b.z;
			//const auto& texID = textureHandles[material];
			//diffuse = Assets::Textures[texID]->SampleUVClamp(uv).ToVec4();
		}
		else {
			ret.uv = vec2(0);
		}
		// Face normals
		//else {
		//	//swizzle_xyz(diffuse) = normalize(cross(p0 - p1, p0 - p2));
		//	//diffuse.a = 1.0f;
		//}

		// Vertex normals interpolated if exist
		if (HasMesh() && Assets::Meshes[meshHandle]->hasNormals) {
			ret.normal = normalize(n0 * b.x + n1 * b.y + n2 * b.z);
			//diffuse = vec4(n0 * b.x + n1 * b.y + n2 * b.z, 1.0f);
		}
		else {
			ret.normal = faceNormal;
		}

		// Loop lights
		//for (const auto& light : lights) {
		//	float atten, nl;
		//	light.CalcGenericLighting(pos, normal, atten, nl);
		//	swizzle_xyz(diffuse) *= nl * atten;
		//}
		//
		//return diffuse;

		return ret;
	}

	//glm::vec4 GetColor(const glm::vec3& pos, const glm::vec3& triNormal, const uint32_t& triIndex, const std::vector<Light>& lights) const {
	//	// Samples the color of this rendered mesh at pos and tri index
	//	
	//
	//};
};