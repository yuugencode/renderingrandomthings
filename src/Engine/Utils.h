#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <bgfx/bgfx.h>

#include <cassert>
#include <fstream>
#include <vector>
#include <filesystem>

// Various utility functions 
namespace Utils {

    // Attempts to load a compiled bgfx shader, hard exits on fail 
    bgfx::ShaderHandle LoadShader(const std::filesystem::path& path);
    
    // Returns barycentric uvw coordinates for the point p interpolated between abc
    glm::vec3 Barycentric(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);

    // Returns barycentric uvw coordinates for the point b, interpolated between abc, confined in the triangle if it was outside
    glm::vec3 ConfinedBarycentric(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);

    // Returns 0...1 value representing how close p is to b compared to a, faux InvLerp in 3D
    float InvSegmentLerp(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b);

    // Returns point p interpolated between a b c d
    glm::vec4 InvQuadrilateral(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d);

    // Checks if vector contains given value 
    template <typename T>
    bool Contains(const std::vector<T>& vec, const T& x) {
        for (size_t i = 0; i < vec.size(); i++) if (vec[i] == x) return true;
        return false;
    }

    // Returns a interpolated towards b using t = 0...1 
    template <typename T>
    T Lerp(const T& a, const T& b, const float& t) {
        return a * (1.0f - t) + b * t;
    }

    // Projects a onto b
    glm::vec3 Project(const glm::vec3& a, const glm::vec3& b);
    
    // Projects vec onto given plane normal
    glm::vec3 ProjectOnPlane(const glm::vec3& vec, const glm::vec3& normal);
    
    // Linear interpolation
    template <typename T>
    T Lerp(const T& a, const T& b, const float& t);
    
    // Returns where val is between min...max as 0...1 percentage 
    float InvLerp(const float& val, const float& min, const float& max);
    
    // Returns where val is between min...max as 0...1 percentage 
    float InvLerpClamp(const float& val, const float& min, const float& max);
    
    // dot(a, a) wrapper
    float SqrLength(const glm::vec3& a);
    
    // Returns a random direction on dir's hemisphere 
    glm::vec3 GenRandomVec(const glm::vec3& dir, const float& hashA, const float& hashB);
    
    // 1 in, 1 out
    float Hash11(float p);
    
    // 3 in, 1 out
    float Hash13(glm::vec3 p3);
    
    // 2 in, 3 out
    glm::vec3 Hash32(const glm::vec2& p);
    
    // 2 in, 2 out
    glm::vec2 Hash22(const glm::vec2& p);

    // 3 in, 2 out
    glm::vec2 Hash23(glm::vec3 p);

    // Constructs a model matrix from given params
    glm::mat4x4 ModelMatrix(const glm::vec3& pos, const glm::vec3& lookAtTarget, const glm::vec3& scale);
    
    // Returns if given enum has
    template <typename T> requires std::is_enum_v<T>
    bool HasFlag(const T& val, const T& flag);

    // Pretty print for a matrix
    void PrintMatrix(const glm::mat4x4& matrix);
};

// Global functions

// Quick hack for swizzling without importing entire glm swizzle catalog 
inline glm::vec3& swizzle_xyz(glm::vec4& vec) {
    return reinterpret_cast<glm::vec3&>(vec);
}
// Quick hack for swizzling without importing entire glm swizzle catalog 
inline const glm::vec3 xyz(const glm::vec4& vec) {
    return vec;
}