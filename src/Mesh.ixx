module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <../3rdparty/ufbx/ufbx.h>
#include <vector> // Intellisense breaks if this is an import...

export module Mesh;

import <memory>;
import <filesystem>;
import <fstream>;
import Log;

export class Mesh {
public:

    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> triangles;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

	Mesh(const std::filesystem::path& path) {

        ufbx_load_opts opts = { 0 }; // Can be null for defaults
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

                ufbx_vec3* meshVerts = node->mesh->vertices.data;
                uint32_t* meshIndices = node->mesh->vertex_indices.data;

                vertices.reserve(vertices.size() + node->mesh->vertices.count);
                triangles.reserve(triangles.size() + node->mesh->max_face_triangles);

                for (size_t i = 0; i < node->mesh->vertices.count; i++) {
                    auto vd = meshVerts[i]; // fbx data is in double
                    auto vf = glm::vec3((float)vd.x, (float)vd.y, (float)vd.z);
                    vertices.push_back(vf);
                }

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
            //else Log::LineFormatted("Object: {}", node->name.data);
        }

        Log::LineFormatted("Read a mesh with {} vertices and {} triangles.", vertices.size(), triangles.size() / 3);

        // Sanity check data
        for (size_t i = 0; i < triangles.size(); i++) {
            if (triangles.at(i) >= vertices.size()) Log::LineFormatted("Invalid triangle index: {}", triangles.at(i));
        }

        ufbx_free_scene(scene);
	}

    void RotateVertices(const glm::quat& rotation) {
        for (size_t i = 0; i < vertices.size(); i++)
            vertices.data()[i] = rotation * vertices.data()[i];
    }

    void ScaleVertices(const float& scale) {
        for (size_t i = 0; i < vertices.size(); i++) {
            vertices.data()[i] = vertices.data()[i] * scale;
        }
    }

    void OffsetVertices(const glm::vec3& offset) {
        for (size_t i = 0; i < vertices.size(); i++) {
            vertices.data()[i] = vertices.data()[i] + offset;
        }
    }

};