#include "Mesh.h"

#include <glm/gtc/quaternion.hpp>

#include "Engine/Utils.h"
#include "Engine/Log.h"

int Mesh::ReadNode(int i, int vertexOffset) {

    // Returns the number of vertices read

    ufbx_node* node = nullptr;
    int cnt = 0;

    // Skip nodes until mesh #i
    for (size_t j = 0; j < scene->nodes.count; j++) {
        node = scene->nodes.data[j];
        if (node->is_root) continue;
        if (!node->mesh) continue;
        if (cnt++ == i) break;
    }

    if (node == nullptr) return 0;

    std::vector<int> seenMaterials;

    ufbx_mesh* mesh = node->mesh;
    ufbx_vec3* meshVerts = mesh->vertices.data;
    uint32_t* meshIndices = mesh->vertex_indices.data;

    vertices.reserve(vertices.size() + mesh->vertices.count);
    triangles.reserve(vertices.size());
    colors.reserve(vertices.size());
    uvs.reserve(vertices.size());
    normals.reserve(vertices.size());
    materials.reserve(vertices.size());
    connectivity.reserve(vertices.size());

    // Vertices, uvs, normals, vertex colors
    for (size_t i = 0; i < mesh->vertex_position.values.count; i++) {
        const auto& vd = mesh->vertex_position.values.data[i];
        vertices.push_back(glm::vec3((float)vd.x, (float)vd.y, (float)vd.z));

        auto firstIndex = mesh->vertex_first_index[i];

        connectivity.emplace_back();
        
        if (mesh->vertex_color.exists && firstIndex != UFBX_NO_INDEX) {
            const auto& clr = mesh->vertex_color.values[mesh->vertex_color.indices[firstIndex]];
            colors.push_back(glm::vec4((float)clr.x, (float)clr.y, (float)clr.z, (float)clr.w));
            hasColors = true;
        }
        else colors.push_back(glm::vec4(1, 1, 1, 1));

        if (mesh->vertex_uv.exists && firstIndex != UFBX_NO_INDEX) {
            const auto& uv = mesh->vertex_uv.values[mesh->vertex_uv.indices[firstIndex]];

            uvs.push_back(glm::vec2((float)uv.x, (float)uv.y));
            hasUVs = true;
        }
        else uvs.push_back(glm::vec2(0, 0));

        if (mesh->vertex_normal.exists && firstIndex != UFBX_NO_INDEX) {
            const auto& nrm = mesh->vertex_normal.values[mesh->vertex_normal.indices[firstIndex]];
            normals.push_back(glm::normalize(glm::vec3((float)nrm.x, (float)nrm.y, (float)nrm.z)));
            hasNormals = true;
        }
        else normals.push_back(glm::vec3(0, 1, 0));
    }

    // Triangles
    for (size_t i = 0; i < mesh->faces.count; i++) {
        auto face = mesh->faces.data[i];

        int matIndex = 0;
        if (mesh->materials.count > 0) {
            const auto& internalId = mesh->face_material.data[i];
            matIndex = mesh->materials.data[internalId].material->typed_id;
            if (matIndex >= scene->materials.count)
                Log::FatalError("Model has tri material indices outside material array length");

            if (!Utils::Contains(seenMaterials, matIndex)) seenMaterials.push_back(matIndex);
        }

        const auto i0 = meshIndices[face.index_begin + 0];
        const auto i1 = meshIndices[face.index_begin + 1];
        const auto i2 = meshIndices[face.index_begin + 2];

        if (std::find(ignoreMaterials.begin(), ignoreMaterials.end(), matIndex) != ignoreMaterials.end())
            continue;

        if (face.num_indices == 3) {
            connectivity[vertexOffset + i0].push_back((uint32_t)triangles.size());
            connectivity[vertexOffset + i1].push_back((uint32_t)triangles.size());
            connectivity[vertexOffset + i2].push_back((uint32_t)triangles.size());

            triangles.push_back(vertexOffset + i0);
            triangles.push_back(vertexOffset + i1);
            triangles.push_back(vertexOffset + i2);
            materials.push_back(matIndex);
        }

        else if (face.num_indices == 4) {
            const auto i3 = meshIndices[face.index_begin + 3];
        
            connectivity[vertexOffset + i0].push_back((uint32_t)triangles.size());
            connectivity[vertexOffset + i1].push_back((uint32_t)triangles.size());
            connectivity[vertexOffset + i2].push_back((uint32_t)triangles.size());
        
            triangles.push_back(vertexOffset + i0);
            triangles.push_back(vertexOffset + i1);
            triangles.push_back(vertexOffset + i2);
            materials.push_back(matIndex);

            connectivity[vertexOffset + i0].push_back((uint32_t)triangles.size());
            connectivity[vertexOffset + i2].push_back((uint32_t)triangles.size());
            connectivity[vertexOffset + i3].push_back((uint32_t)triangles.size());
        
            triangles.push_back(vertexOffset + i0);
            triangles.push_back(vertexOffset + i2);
            triangles.push_back(vertexOffset + i3);
            materials.push_back(matIndex);
        }
    }

    numUniqueMaterials = (int)seenMaterials.size();

#if false // Debugging
    // Update triangle connectivity
    triConnectivity.resize(triangles.size() / 3);

    for (size_t vi = 0; vi < vertices.size(); vi++) {
        // For every vertice, loop each triangle it has and add to every neighbor if not present
        const auto& vertNeighborTris = connectivity[vi];
        for (const auto& tri : vertNeighborTris) {
            auto& triNeighborTris = triConnectivity[tri / 3];
            for (const auto& otherTri : vertNeighborTris) {
                if (otherTri == tri) continue;
                if (Utils::Contains(triNeighborTris, otherTri)) continue;
                triNeighborTris.push_back(otherTri);
            }
        }
    }
#endif

    return (int)mesh->vertices.count;
}

void Mesh::CheckData() {

    if (!sanityCheckData) return;
    
    if (vertices.size() == 0) Log::Line("Checking mesh with no vertices?");

    // Sanity check data
    for (size_t i = 0; i < triangles.size(); i++)
        if (triangles.at(i) >= vertices.size())
            Log::FatalError("Invalid triangle index: {}", triangles.at(i));

    if ((vertices.size() != uvs.size()) || (uvs.size() != colors.size()) || (colors.size() != normals.size()))
        Log::FatalError("Mesh array sizes don't match");
}

void Mesh::LoadMesh(const std::filesystem::path& path, bool loadMtl) {
    ufbx_load_opts opts = { };

    if (loadMtl) opts.obj_search_mtl_by_filename = true;

    ufbx_error error; // Can be null to disable errors
    scene = ufbx_load_file(path.string().c_str(), &opts, &error);
    if (!scene) Log::FatalError("Failed to load: ", error.description.data);
}

void Mesh::UnloadMesh() {
    if (scene != nullptr) ufbx_free_scene(scene);
}

void Mesh::Clear() {
    connectivity.clear();
    triConnectivity.clear();
    materialMetadata.clear();
    vertices.clear();
    triangles.clear();
    materials.clear();
    uvs.clear();
    normals.clear();
    colors.clear();
    hasColors = hasNormals = hasUVs = false;
    numUniqueMaterials = 0;
}

void Mesh::ReadTextures() {
    for (size_t i = 0; i < scene->materials.count; i++) {
        auto mat = scene->materials.data[i];

        if (mat->textures.count == 0) {
            materialMetadata.push_back(MaterialMetadata{
                .materialName = std::string(mat->name.data, mat->name.length),
                .textureFilename = ""
            });
            continue;
        }

        auto tex = mat->textures.data[0].texture;
        auto path = std::string(tex->filename.data, tex->filename.length);
        auto filename = std::filesystem::path(path).filename().replace_extension("png");
        materialMetadata.push_back(MaterialMetadata{
                .materialName = std::string(mat->name.data, mat->name.length),
                .textureFilename = filename.string()
        });
    }
}

void Mesh::ReadAllNodes() {
    if (scene == nullptr)
        Log::FatalError("Reading scene with no scene loaded");

    Clear();

    ReadTextures();

    int meshCount = GetMeshNodeCount();
    int vertexCnt = 0;
    for (size_t i = 0; i < meshCount; i++)
        vertexCnt += ReadNode((int)i, vertexCnt);
    CheckData();

    Log::LineFormatted("Read a mesh with {} vertices, {} triangles and {} materials.", vertices.size(), triangles.size() / 3, scene->materials.count);
    Log::LineFormatted("Has UVs: {}, Has Normals: {}, Has Colors: {}", hasUVs, hasNormals, hasColors);
}

void Mesh::ReadSceneMeshNode(int nodeIndex) {
    if (scene == nullptr)
        Log::FatalError("Reading scene with no scene loaded");

    Clear();

    if (nodeIndex < 0 || nodeIndex >= scene->nodes.count)
        Log::FatalError("Tried to read a mesh file node past file scene indices");

    ReadTextures();

    ReadNode(nodeIndex, 0);
    CheckData();

    Log::LineFormatted("Read a mesh with {} vertices, {} triangles and {} materials.", vertices.size(), triangles.size() / 3, scene->materials.count);
    Log::LineFormatted("Has UVs: {}, Has Normals: {}, Has Colors: {}", hasUVs, hasNormals, hasColors);
}

int Mesh::GetMeshNodeCount() {

    if (scene == nullptr) { Log::FatalError("No mesh loaded?"); return 0; }

    int cnt = 0;
    for (size_t i = 0; i < scene->nodes.count; i++)
        if (!scene->nodes[i]->is_root && scene->nodes[i]->mesh)
            cnt++;

    return cnt;
}

void Mesh::RotateVertices(const glm::quat& rotation) {
    for (size_t i = 0; i < vertices.size(); i++) {
        vertices.data()[i] = rotation * vertices.data()[i];
        normals.data()[i] = rotation * normals.data()[i];
    }
}

void Mesh::ScaleVertices(const float& scale) {
    for (size_t i = 0; i < vertices.size(); i++)
        vertices.data()[i] = vertices.data()[i] * scale;
}

void Mesh::OffsetVertices(const glm::vec3& offset) {
    for (size_t i = 0; i < vertices.size(); i++)
        vertices.data()[i] = vertices.data()[i] + offset;
}

void Mesh::CopyFrom(const Mesh& other) {
    vertices = other.vertices;
    triangles = other.triangles;
    colors = other.colors;
    uvs = other.uvs;
    normals = other.normals;
    materials = other.materials;
    hasNormals = other.hasNormals;
    hasColors = other.hasColors;
    hasUVs = other.hasUVs;
}
