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
public:

    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> triangles;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> colors;

    bool hasColors = false, hasNormals = false, hasUVs = false;

	Mesh(const std::filesystem::path& path) {

        ufbx_load_opts opts = { 0 };
        ufbx_error error; // Can be null to disable errors
        ufbx_scene* scene = ufbx_load_file(path.string().c_str(), &opts, &error);
        
        if (!scene) Log::FatalError("Failed to load: ", error.description.data);

        vertices = std::vector<glm::vec3>();
        triangles = std::vector<uint32_t>();

        uint32_t vertexCnt = 0;

        // Loop everything in the scene
        for (size_t i = 0; i < scene->nodes.count; i++) {
            ufbx_node* node = scene->nodes.data[i];
            if (node->is_root) continue;

            // Grab vertices / triangles from meshes
            if (node->mesh) {
                //Log::LineFormatted("Mesh: {} with {} faces", node->name.data, node->mesh->faces.count);

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
                for (size_t i = 0; i < node->mesh->faces.count; i++) {
                    auto face = node->mesh->faces.data[i];

                    if (face.num_indices == 3) {
                        triangles.push_back(vertexCnt + meshIndices[face.index_begin + 0]);
                        triangles.push_back(vertexCnt + meshIndices[face.index_begin + 1]);
                        triangles.push_back(vertexCnt + meshIndices[face.index_begin + 2]);
                    }

                    else if (face.num_indices == 4) {
                        triangles.push_back(vertexCnt + meshIndices[face.index_begin + 0]);
                        triangles.push_back(vertexCnt + meshIndices[face.index_begin + 1]);
                        triangles.push_back(vertexCnt + meshIndices[face.index_begin + 2]);

                        triangles.push_back(vertexCnt + meshIndices[face.index_begin + 2]);
                        triangles.push_back(vertexCnt + meshIndices[face.index_begin + 3]);
                        triangles.push_back(vertexCnt + meshIndices[face.index_begin + 0]);
                    }
                }

                vertexCnt += (uint32_t)node->mesh->vertices.count;
            }
        }

        // Sanity check data
        for (size_t i = 0; i < triangles.size(); i++)
            if (triangles.at(i) >= vertices.size())
                Log::LineFormatted("Invalid triangle index: {}", triangles.at(i));

        assert(vertices.size() == uvs.size() && uvs.size() == colors.size() && colors.size() == normals.size());
        
        Log::LineFormatted("Read a mesh with {} vertices and {} triangles.", vertices.size(), triangles.size() / 3);
        Log::LineFormatted("Has UVs: {}, Has Normals: {}, Has Colors: {}", hasUVs, hasNormals, hasColors);

        ufbx_free_scene(scene);
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
};