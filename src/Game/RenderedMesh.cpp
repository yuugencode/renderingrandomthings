#include "RenderedMesh.h"

#include <filesystem>

#include "Engine/Timer.h"
#include "Engine/Log.h"
#include "Rendering/RayResult.h"

RenderedMesh::RenderedMesh(const std::string& name, const int& meshHandle) {
	this->type = Entity::Type::RenderedMesh;
	this->meshHandle = meshHandle;
	this->name = name;
	id = idCount--;
	SetShader(Shader::Textured);
	GenerateBVH();
	materials.push_back(Material());
}

void RenderedMesh::GenerateBVH() {

	// Quick hack for serializing a bvh // @TODO: Refactor
	std::string filename = name + ".bvh";

	if (name.length() > 0 && std::filesystem::exists(filename)) {
		// BVH Exists on disk, read
		Log::LineFormatted("Reading bvh from file: {}", filename);
		std::ifstream ifs(filename, std::ios::binary);
		int stackCount;
		ifs.read((char*)&stackCount, sizeof(int));
		bvh.stack.resize(stackCount);
		ifs.read((char*)bvh.stack.data(), stackCount * sizeof(Bvh::BvhNode));
		int triCount;
		ifs.read((char*)&triCount, sizeof(int));
		if (triCount != GetMesh()->triangles.size() / 3) Log::FatalError("Invalid BVH on disk:", filename);
		bvh.triangles.resize(triCount);
		ifs.read((char*)bvh.triangles.data(), triCount * sizeof(Bvh::BvhTriangle));
		aabb = bvh.stack[0].aabb;
	}
	else {
		// BVH Doesn't exist on disk have to generate
		if (name.length() == 0) Log::Line("Generating BVH for unnamed obj");
		else Log::LineFormatted("Generating bvh because {} doesn't exist", filename);
		Timer t(1);
		t.Start();
		bvh.Generate(GetMesh()->vertices, GetMesh()->triangles);
		aabb = bvh.stack[0].aabb;
		t.End();
		Log::LineFormatted("BVH Generation time {}ms", Log::FormatFloat((float)t.GetAveragedTime() * 1000.0f));
		if (name.length() > 0) {
			std::ofstream ofs(filename, std::ios::binary);
			int stackCount = (int)bvh.stack.size();
			ofs.write((char*)&stackCount, sizeof(int));
			ofs.write((char*)bvh.stack.data(), stackCount * sizeof(Bvh::BvhNode));
			int triCount = (int)bvh.triangles.size();
			ofs.write((char*)&triCount, sizeof(int));
			ofs.write((char*)bvh.triangles.data(), triCount * sizeof(Bvh::BvhTriangle));
			Log::LineFormatted("Wrote BVH to disk ({}Mb)", ((bvh.stack.size() * sizeof(Bvh::BvhNode)) / 1024) / 1024);
		}
	}
}

bool RenderedMesh::IntersectLocal(const Ray& ray, glm::vec3& normal, int& triIdx, float& depth) const {
	return bvh.Intersect(ray, normal, triIdx, depth);
}

v2f RenderedMesh::VertexShader(const Ray& ray, const RayResult& rayResult) const {
	v2f ret;

	const auto& mesh = Assets::Meshes[meshHandle];
	const auto& v0i = mesh->triangles[rayResult.triIndex + 0];
	const auto& v1i = mesh->triangles[rayResult.triIndex + 1];
	const auto& v2i = mesh->triangles[rayResult.triIndex + 2];
	const auto& p0 = mesh->vertices[v0i];
	const auto& p1 = mesh->vertices[v1i];
	const auto& p2 = mesh->vertices[v2i];
	//const auto& c0 = mesh->colors[v0i];
	//const auto& c1 = mesh->colors[v1i];
	//const auto& c2 = mesh->colors[v2i];
	const auto& n0 = mesh->normals[v0i];
	const auto& n1 = mesh->normals[v1i];
	const auto& n2 = mesh->normals[v2i];
	const auto& uv0 = mesh->uvs[v0i];
	const auto& uv1 = mesh->uvs[v1i];
	const auto& uv2 = mesh->uvs[v2i];
	//const auto& material = mesh->materialIDs[rayResult.triIndex / 3];

	// Barycentric interpolation
	glm::vec3 b = Utils::Barycentric(rayResult.localPos, p0, p1, p2);

	ret.worldPosition = ray.ro + ray.rd * rayResult.depth;
	ret.rayDirection = ray.rd;

	ret.uv = uv0 * b.x + uv1 * b.y + uv2 * b.z;

	// Vertex normals interpolated if exist
	if (HasMesh() && Assets::Meshes[meshHandle]->hasNormals)
		ret.localNormal = normalize(n0 * b.x + n1 * b.y + n2 * b.z);
	else
		ret.localNormal = rayResult.faceNormal;

	ret.worldNormal = transform.rotation * ret.localNormal;

	return ret;
}

Color RenderedMesh::SampleAt(const glm::vec3& pos, int triIndex) const {
	if (!HasMesh()) return Color{ .r = 0x00, .g = 0x00, .b = 0x00, .a = 0xff };

	const auto& mesh = GetMesh();
	const auto& materialID = mesh->materialIDs[triIndex / 3];
	const auto& material = materials[materialID];
	if (!material.HasTexture()) return Colors::Black;

	const auto& v0i = mesh->triangles[triIndex + 0];
	const auto& v1i = mesh->triangles[triIndex + 1];
	const auto& v2i = mesh->triangles[triIndex + 2];
	const auto& p0 = mesh->vertices[v0i];
	const auto& p1 = mesh->vertices[v1i];
	const auto& p2 = mesh->vertices[v2i];
	const auto& uv0 = mesh->uvs[v0i];
	const auto& uv1 = mesh->uvs[v1i];
	const auto& uv2 = mesh->uvs[v2i];

	const glm::vec3 b = Utils::Barycentric(pos, p0, p1, p2);
	const auto uv = uv0 * b.x + uv1 * b.y + uv2 * b.z;

	return Assets::Textures[material.textureHandle]->SampleUVClamp(uv);
}

Color RenderedMesh::SampleTriangle(int triIndex, const glm::vec3& barycentric) const {
	if (!HasMesh()) return Color{ .r = 0x00, .g = 0x00, .b = 0x00, .a = 0xff };

	const auto& mesh = GetMesh();
	const auto& materialID = mesh->materialIDs[triIndex / 3];
	const auto& texID = materials[materialID].textureHandle;
	if (texID == -1) return Colors::Clear;

	const auto& v0i = mesh->triangles[triIndex + 0];
	const auto& v1i = mesh->triangles[triIndex + 1];
	const auto& v2i = mesh->triangles[triIndex + 2];
	const auto& uv0 = mesh->uvs[v0i];
	const auto& uv1 = mesh->uvs[v1i];
	const auto& uv2 = mesh->uvs[v2i];

	const auto uv = uv0 * barycentric.x + uv1 * barycentric.y + uv2 * barycentric.z;
	return Assets::Textures[texID]->SampleUVClamp(uv);
}

