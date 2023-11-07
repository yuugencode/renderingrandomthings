#include "RenderedMesh.h"

#include "Engine/Timer.h"
#include "Engine/Log.h"
#include "Rendering/RayResult.h"

RenderedMesh::RenderedMesh(const int& meshHandle) {
	this->type = Entity::Type::RenderedMesh;
	this->meshHandle = meshHandle;
	id = idCount--;
	shaderType = Shader::Textured;
	GenerateBVH();
}

void RenderedMesh::GenerateBVH() {
	// Construct a bvh
	Timer t(1);
	t.Start();
	bvh.Generate(GetMesh()->vertices, GetMesh()->triangles);
	aabb = bvh.stack[0].aabb;
	t.End();
	Log::Line("bvh gen", Log::FormatFloat((float)t.GetAveragedTime() * 1000.0f), "ms");
}

bool RenderedMesh::IntersectLocal(const Ray& ray, glm::vec3& normal, int& triIdx, float& depth) const {
	return bvh.Intersect(ray, normal, triIdx, depth);
}

v2f RenderedMesh::VertexShader(const glm::vec3& worldPos, const RayResult& rayResult) const {
	using namespace glm; // Pretend we're a shader

	v2f ret;

	const auto& mesh = Assets::Meshes[meshHandle];
	const auto& v0i = mesh->triangles[rayResult.data + 0]; // data == triIndex for meshes
	const auto& v1i = mesh->triangles[rayResult.data + 1];
	const auto& v2i = mesh->triangles[rayResult.data + 2];
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
	const auto& material = mesh->materials[rayResult.data / 3];

	// Barycentric interpolation
	const auto b = Utils::Barycentric(rayResult.localPos, p0, p1, p2);

	ret.worldPosition = worldPos;

	// Texture sample
	if (HasTexture())
		ret.uv = uv0 * b.x + uv1 * b.y + uv2 * b.z;
	else
		ret.uv = vec2(0);

	// Vertex normals interpolated if exist
	if (HasMesh() && Assets::Meshes[meshHandle]->hasNormals)
		ret.localNormal = normalize(n0 * b.x + n1 * b.y + n2 * b.z);
	else
		ret.localNormal = rayResult.faceNormal;

	return ret;
}
