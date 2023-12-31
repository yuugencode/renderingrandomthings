#pragma once

#include <glm/glm.hpp>

// Contains common standalone structs used all over for various things

// Raycasting ray
struct Ray {
    glm::vec3 ro, rd, inv_rd;
    int mask;
};

// Color structure for 4 byte colors
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 0;

    // Turns floats (0...1) to a byte color 
    static Color FromFloat(const float& r, const float& g, const float& b, const float& a);
    
    // Turns a float (0...1) to rgb byte color with 1.0 alpha 
    static Color FromFloat(const float& x);
    
    // Turns a float vector (0...1) to a byte color 
    static Color FromVec(const glm::vec3& v, const float& alpha);
    
    // Turns a float vector (0...1) to a byte color 
    static Color FromVec(const glm::vec4& v);
    
    // Lerps color towards another using t = 0...1 
    static Color Lerp(const Color& a, const Color& b, const float& t);
    
    // Lerps color towards another using t = 0...1 
    Color Lerp(const Color& other, const float& t) const;
    
    glm::vec4 ToVec4() const;
};

// Simple axis-aligned bounding box
struct AABB {
    glm::vec3 min, max;

    AABB();
    AABB(const float& size);
    AABB(const glm::vec3& min, const glm::vec3& max);
    AABB(const glm::vec3& pos);
    AABB(const glm::vec3& pos, const float& radius);
    glm::vec3 Center() const;
    glm::vec3 Size() const;

    // Returns if this AABB contains given point
    bool Contains(const glm::vec3& pt) const;

    // Grows the size of AABB to contain given point
    void Encapsulate(const glm::vec3& point);

    // Returns the SAH area heuristic for this AABB
    float AreaHeuristic() const;

    // Intersects a ray with this AABB, returns 0.0f on no hit, negative if behind and positive depth on hit
    float Intersect(const Ray& ray) const;

    // Returns the closest point on this AABB to given pos
    glm::vec3 ClosestPoint(const glm::vec3& pos) const;

    // Returns the squared distance from given pt to the closest point on this AABB
    float SqrDist(const glm::vec3& pos) const;

    // Returns one of the i=0...7 vertices of this AABB
    glm::vec4 GetVertice(int i) const;
};

// Hardcoded CPU Shader
enum class Shader {
    PlainWhite, Normals, Textured, Grid, Debug
};

// Vertex interpolator output sent to ""fragment shader""
struct v2f {
    glm::vec3 worldPosition;
    glm::vec3 localNormal;
    glm::vec3 worldNormal;
    glm::vec3 rayDirection;
    glm::vec2 uv;
};

// Light's indirect buffer BVH payload struct
struct LightbufferPayload {
    Color clr; // Indirect color sum
    glm::vec3 nrm; // Receiver point world normal
};

// Light's indirect buffer contents
struct LightbufferPt {
    glm::vec3 pt; // World position
    LightbufferPayload indirect;
};

struct Empty {
    // Empty, used for "no-payload" in templated structs
};

// Global constants
namespace Colors {
    inline constexpr Color Red = Color(0xff, 0x00, 0x00, 0xff);
    inline constexpr Color Green = Color(0x00, 0xff, 0x00, 0xff);
    inline constexpr Color Blue = Color(0x00, 0x00, 0xff, 0xff);
    inline constexpr Color White = Color(0xff, 0xff, 0xff, 0xff);
    inline constexpr Color Black = Color(0x00, 0x00, 0x00, 0xff);
    inline constexpr Color Clear = Color(0x00, 0x00, 0x00, 0x00);
    inline constexpr Color Cyan = Color(0x00, 0xff, 0xff, 0xff);
}

inline constexpr float PI = 3.14159265358979f;

