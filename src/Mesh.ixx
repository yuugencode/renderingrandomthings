module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <../3rdparty/ufbx/ufbx.h>
#include <vector> // Intellisense breaks if this is an import...

export module Mesh;

import <memory>;
import <filesystem>;
import <fstream>;
import Utils;

export class Mesh {

    ufbx_scene* scene;

    int ReadNode(int i, int vertexOffset = 0) {

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

        ufbx_mesh* mesh = node->mesh;
        ufbx_vec3* meshVerts = mesh->vertices.data;
        uint32_t* meshIndices = mesh->vertex_indices.data;

        vertices.reserve(vertices.size() + mesh->vertices.count);
        triangles.reserve(triangles.size() + mesh->max_face_triangles);
        colors.reserve(vertices.size());
        uvs.reserve(vertices.size());
        normals.reserve(vertices.size());

        // Vertices, uvs, normals, vertex colors
        for (size_t i = 0; i < mesh->vertex_position.values.count; i++) {
            const auto& vd = mesh->vertex_position.values.data[i];
            vertices.push_back(glm::vec3((float)vd.x, (float)vd.y, (float)vd.z));

            auto firstIndex = mesh->vertex_first_index[i];

            if (mesh->vertex_color.exists) {
                const auto& clr = mesh->vertex_color.values[mesh->vertex_color.indices[firstIndex]];
                colors.push_back(glm::vec4((float)clr.x, (float)clr.y, (float)clr.z, (float)clr.w));
                hasColors = true;
            }
            else colors.push_back(glm::vec4(0, 0, 0, 0));

            if (mesh->vertex_uv.exists) {
                const auto& uv = mesh->vertex_uv.values[mesh->vertex_uv.indices[firstIndex]];
                uvs.push_back(glm::vec2((float)uv.x, (float)uv.y));
                hasUVs = true;
            }
            else uvs.push_back(glm::vec2(0, 0));

            if (mesh->vertex_normal.exists) {
                const auto& nrm = mesh->vertex_normal.values[mesh->vertex_normal.indices[firstIndex]];
                normals.push_back(glm::normalize(glm::vec3((float)nrm.x, (float)nrm.y, (float)nrm.z)));
                hasNormals = true;
            }
            else normals.push_back(glm::vec3(0, 1, 0));
        }

        // Triangles
        for (size_t i = 0; i < mesh->faces.count; i++) {
            auto face = mesh->faces.data[i];

            uint32_t matIndex = -1;
            if (mesh->materials.count > 0) {
                const auto& internalId = mesh->face_material.data[i];
                matIndex = mesh->materials.data[internalId].material->typed_id;
                if (matIndex >= scene->materials.count)
                    Log::FatalError("Model has tri material indices outside material array length");
            }

            if (face.num_indices == 3) {
                triangles.push_back(vertexOffset + meshIndices[face.index_begin + 0]);
                triangles.push_back(vertexOffset + meshIndices[face.index_begin + 1]);
                triangles.push_back(vertexOffset + meshIndices[face.index_begin + 2]);

                materials.push_back(matIndex);
            }

            else if (face.num_indices == 4) {
                triangles.push_back(vertexOffset + meshIndices[face.index_begin + 0]);
                triangles.push_back(vertexOffset + meshIndices[face.index_begin + 1]);
                triangles.push_back(vertexOffset + meshIndices[face.index_begin + 2]);

                triangles.push_back(vertexOffset + meshIndices[face.index_begin + 0]);
                triangles.push_back(vertexOffset + meshIndices[face.index_begin + 2]);
                triangles.push_back(vertexOffset + meshIndices[face.index_begin + 3]);

                materials.push_back(matIndex);
                materials.push_back(matIndex);
            }
        }
        
        return (int)mesh->vertices.count;
    }

    void CheckData() {
        
        if (vertices.size() == 0) Log::Line("Checking mesh with no vertices?");
        
        // Sanity check data
        for (size_t i = 0; i < triangles.size(); i++)
            if (triangles.at(i) >= vertices.size())
                Log::FatalError("Invalid triangle index: {}", triangles.at(i));

        if ((vertices.size() != uvs.size()) || (uvs.size() != colors.size()) || (colors.size() != normals.size()))
            Log::FatalError("Mesh array sizes don't match");
    }

public:

    Mesh() {}

    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> triangles;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> colors;
    std::vector<uint32_t> materials; // tri index -> material index

    bool hasColors = false, hasNormals = false, hasUVs = false;

    void LoadMesh(const std::filesystem::path& path) {
        ufbx_load_opts opts = { 0 };
        ufbx_error error; // Can be null to disable errors
        scene = ufbx_load_file(path.string().c_str(), &opts, &error);
        if (!scene) Log::FatalError("Failed to load: ", error.description.data);
    }

    void UnloadMesh() {
        if (scene != nullptr) ufbx_free_scene(scene);
    }

    void Clear() {
        vertices.clear();
        triangles.clear();
        uvs.clear();
        normals.clear();
        colors.clear();
    }

    void ReadAllNodes() {
        if (scene == nullptr)
            Log::FatalError("Reading scene with no scene loaded");

        Clear();

        int meshCount = GetMeshNodeCount();
        int vertexCnt = 0;
        for (size_t i = 0; i < meshCount; i++)
            vertexCnt += ReadNode((int)i, vertexCnt);
        CheckData();

        Log::LineFormatted("Read a mesh with {} vertices and {} triangles.", vertices.size(), triangles.size() / 3);
        Log::LineFormatted("Has UVs: {}, Has Normals: {}, Has Colors: {}", hasUVs, hasNormals, hasColors);
    }

    void ReadSceneMeshNode(int nodeIndex) {
        if (scene == nullptr) 
            Log::FatalError("Reading scene with no scene loaded");
        
        Clear();
        
        if (nodeIndex < 0 || nodeIndex >= scene->nodes.count)
            Log::FatalError("Tried to read a mesh file node past file scene indices");

        ReadNode(nodeIndex);
        CheckData();

        Log::LineFormatted("Read a mesh with {} vertices and {} triangles.", vertices.size(), triangles.size() / 3);
        Log::LineFormatted("Has UVs: {}, Has Normals: {}, Has Colors: {}", hasUVs, hasNormals, hasColors);
    }

    int GetMeshNodeCount() {

        if (scene == nullptr) Log::FatalError("No mesh loaded?");

        int cnt = 0;
        for (size_t i = 0; i < scene->nodes.count; i++)
            if (!scene->nodes[i]->is_root && scene->nodes[i]->mesh)
                cnt++;

        return cnt;
    }

    /// <summary> Bakes rotational offset to data </summary>
    void RotateVertices(const glm::quat& rotation) {
        for (size_t i = 0; i < vertices.size(); i++) {
            vertices.data()[i] = rotation * vertices.data()[i];
            normals.data()[i] = rotation * normals.data()[i];
        }
    }

    /// <summary> Bakes scale offset to data </summary>
    void ScaleVertices(const float& scale) {
        for (size_t i = 0; i < vertices.size(); i++)
            vertices.data()[i] = vertices.data()[i] * scale;
    }

    /// <summary> Bakes positional offset to data </summary>
    void OffsetVertices(const glm::vec3& offset) {
        for (size_t i = 0; i < vertices.size(); i++)
            vertices.data()[i] = vertices.data()[i] + offset;
    }

    /// <summary> Copies the data from other mesh </summary>
    void CopyFrom(const Mesh& other) {
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
};