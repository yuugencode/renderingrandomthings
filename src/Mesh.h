#pragma once

#include <glm/glm.hpp>
#include "../3rdparty/ufbx/ufbx.h"
#include <vector>
#include <filesystem>

// A triangle based mesh
class Mesh {

    ufbx_scene* scene = nullptr;

    int ReadNode(int i, int vertexOffset = 0);
    
    void CheckData();

public:
    Mesh() = default;

    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> triangles;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> colors;
    std::vector<uint32_t> materials; // tri index -> material index
    std::vector<std::vector<uint32_t>> connectivity; // vertex index -> triangle list
    std::vector<std::vector<uint32_t>> triConnectivity; // triangle index -> neighbor triangles

    bool hasColors = false, hasNormals = false, hasUVs = false;

    // Loads fbx/obj mesh from given path into ufbx structure
    void LoadMesh(const std::filesystem::path& path);
    
    // Unloads the internal ufbx representation
    void UnloadMesh();

    // Clears this mesh data
    void Clear();

    // Reads all nodes from the loaded fbx and creates one merged mesh from them
    void ReadAllNodes();

    // Reads a single mesh node from the loaded fbx and creates a mesh from it
    void ReadSceneMeshNode(int nodeIndex);
    
    // Returns the number of meshes in the loaded fbx scene
    int GetMeshNodeCount();

    // Bakes rotation to loaded data
    void RotateVertices(const glm::quat& rotation);
    
    // Bakes scaling to loaded data
    void ScaleVertices(const float& scale);
    
    // Bakes translation to loaded data
    void OffsetVertices(const glm::vec3& offset);
    
    // Makes a copy of this mesh from other
    void CopyFrom(const Mesh& other);
};
