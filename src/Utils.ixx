module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <bgfx/bgfx.h>
#include <cassert>
#include <fstream>

export module Utils;

import <vector>;
import <filesystem>;

import Log;

/// <summary> Color structure for 4 byte colors </summary>
export struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 0;
    
    Color() {}
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        this->r = r; this->g = g; this->b = b; this->a = a;
    }
    /// <summary> Turns floats (0...1) to a byte color </summary>
    static auto FromFloat(const float& r, const float& g, const float& b, const float& a) {
        Color c;
        c.r = static_cast<uint8_t>(glm::clamp(r * 255.0f, 0.0f, 255.0f));
        c.g = static_cast<uint8_t>(glm::clamp(g * 255.0f, 0.0f, 255.0f)); 
        c.b = static_cast<uint8_t>(glm::clamp(b * 255.0f, 0.0f, 255.0f)); 
        c.a = static_cast<uint8_t>(glm::clamp(a * 255.0f, 0.0f, 255.0f));
        return c;
    }
    /// <summary> Turns a float (0...1) to rgb byte color with 1.0 alpha </summary>
    static auto FromFloat(const float& x) {
        Color c;
        c.r = static_cast<uint8_t>(glm::clamp(x * 255.0f, 0.0f, 255.0f));
        c.g = static_cast<uint8_t>(glm::clamp(x * 255.0f, 0.0f, 255.0f));
        c.b = static_cast<uint8_t>(glm::clamp(x * 255.0f, 0.0f, 255.0f));
        c.a = 255;
        return c;
    }

    /// <summary> Turns a float vector (0...1) to a byte color </summary>
    static auto FromVec(glm::vec3 v, const float& alpha) {
        v = glm::clamp(v * 255.0f, 0.0f, 255.0f);
        Color c;
        c.r = static_cast<uint8_t>(v.x);
        c.g = static_cast<uint8_t>(v.y);
        c.b = static_cast<uint8_t>(v.z);
        c.a = static_cast<uint8_t>(alpha);
        return c;
    }

    /// <summary> Turns a float vector (0...1) to a byte color </summary>
    static auto FromVec(glm::vec4 v) {
        v = glm::clamp(v * 255.0f, 0.0f, 255.0f);
        Color c;
        c.r = static_cast<uint8_t>(v.x);
        c.g = static_cast<uint8_t>(v.y);
        c.b = static_cast<uint8_t>(v.z);
        c.a = static_cast<uint8_t>(v.w);
        return c;
    }

    /// <summary> Lerps color towards another using t = 0...1 </summary>
    static Color Lerp(const Color& a, const Color& b, float t) {
        Color c;
        c.r = (int)(a.r * (1.0f - t) + b.r * t);
        c.g = (int)(a.g * (1.0f - t) + b.g * t);
        c.b = (int)(a.b * (1.0f - t) + b.b * t);
        c.a = (int)(a.a * (1.0f - t) + b.a * t);
        return c;
    }

    /// <summary> Lerps color towards another using t = 0...1 </summary>
    Color Lerp(const Color& other, float t) {
        return Color::Lerp(*this, other, t);
    }

    Color operator+(const Color& c) {
        Color ret;
        ret.r = r + c.r; ret.g = r + c.g; ret.b = r + c.b; ret.a = r + c.a;
        return ret;
    }

    Color& operator+=(const Color& rhs) {
        r += rhs.r; g += rhs.g; b += rhs.b; a += rhs.a;
        return *this;
    }

    auto operator<=>(const Color&) const = default;
};

// Make log work for color
export std::ostream& operator<<(std::ostream& target, const Color& c) {
    // uint8_t can't be <<'d
    target << '(' << (int)c.r << ", " << (int)c.g << ", " << (int)c.b << ", " << (int)c.a << ')';
    return target;
}
export std::ostream& operator<<(std::ostream& target, const uint8_t& c) {
    target << (int)c; // uint8_t can't be <<'d
    return target;
}

/// <summary> Simple axis-aligned bounding box </summary>
export struct AABB {
    glm::vec3 min, max;

    AABB() {}
    AABB(const glm::vec3& min, const glm::vec3& max) {
        this->min = min; this->max = max;
    }

    AABB(const glm::vec3& pos) {
        this->min = pos; this->max = pos;
    }

    glm::vec3 Center() const { return max * 0.5f + min * 0.5f; }
    glm::vec3 Size() const { return max - min; }

    void Encapsulate(const glm::vec3& point) {
        min = glm::min(point, min);
        max = glm::max(point, max);
    }

    // Slab method
    float Intersect(const glm::vec3& ro, const glm::vec3& rd) const {
        auto invDir = 1.0f / rd;
        auto a = (min - ro) * invDir;
        auto b = (max - ro) * invDir;
        auto mi = glm::min(a, b);
        auto ma = glm::max(a, b);
        auto minT = glm::max(glm::max(mi.x, mi.y), mi.z);
        auto maxT = glm::min(glm::min(ma.x, ma.y), ma.z);
        if (maxT < 0.0f) return 0.0f;
        if (minT > maxT) return 0.0f;
        return minT;
    }
};

/// <summary> Various utility functions </summary>
export namespace Utils {

    /// <summary> Attempts to load a compiled bgfx shader, hard exits on fail </summary>
    bgfx::ShaderHandle LoadShader(const std::filesystem::path& path) {
        assert(std::filesystem::exists(path));
        
        std::ifstream file = std::ifstream(path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        const bgfx::Memory* buffer = bgfx::alloc(uint32_t(size + 1));
        if (file.read((char*)buffer->data, size)) {
            buffer->data[buffer->size - 1] = '\0';
        }
        
        assert(buffer != nullptr);
        return bgfx::createShader(buffer);
    }

    /// <summary> Projects vector a onto b </summary>
    glm::vec3 Project(const glm::vec3& a, const glm::vec3& b) { 
        return b * glm::dot(a, b); 
    }

    /// <summary> Projects vector vec onto plane </summary>
    glm::vec3 ProjectOnPlane(const glm::vec3& vec, const glm::vec3& normal) {
        return vec - Project(vec, normal);
    }

    /// <summary> Returns where val is between min...max as 0...1 percentage </summary>
    float InvLerp(const float& val, const float& min, const float& max) {
        return (val - min) / (max - min); 
    }
    /// <summary> Returns where val is between min...max as 0...1 percentage </summary>
    float InvLerpClamp(const float& val, const float& min, const float& max) {
        return InvLerp(glm::clamp(val, min, max), min, max);
    }

    /// <summary> Constructs a model matrix from given pos/fwd/up/scale </summary>
    glm::mat4x4 ModelMatrix(const glm::vec3& pos, const glm::vec3& lookAtTarget, const glm::vec3& scale) {
        auto mat_pos = glm::translate(glm::mat4x4(1.0f), pos);
        auto mat_rot = glm::mat4x4(glm::quatLookAt(glm::normalize(lookAtTarget - pos), glm::vec3(0, 1, 0)));
        auto mat_scl = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
        return mat_pos * mat_rot * mat_scl;
    }

    void PrintMatrix(const glm::mat4x4& matrix) {
        Log::Line("Matrix4x4:");
        Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][0]), Log::FormatFloat(matrix[1][0]), Log::FormatFloat(matrix[2][0]), Log::FormatFloat(matrix[3][0]));
        Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][1]), Log::FormatFloat(matrix[1][1]), Log::FormatFloat(matrix[2][1]), Log::FormatFloat(matrix[3][1]));
        Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][2]), Log::FormatFloat(matrix[1][2]), Log::FormatFloat(matrix[2][2]), Log::FormatFloat(matrix[3][2]));
        Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][3]), Log::FormatFloat(matrix[1][3]), Log::FormatFloat(matrix[2][3]), Log::FormatFloat(matrix[3][3]));
    }
};

export namespace Globals {
    export const Color Red = Color(0xff, 0x00, 0x00, 0xff);
    export const Color Green = Color(0x00, 0xff, 0x00, 0xff);
    export const Color Blue = Color(0x00, 0x00, 0xff, 0xff);
    export const Color White = Color(0xff, 0xff, 0xff, 0xff);
    export const Color Black = Color(0x00, 0x00, 0x00, 0xff);
}