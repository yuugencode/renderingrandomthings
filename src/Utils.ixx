module;

#include <glm/glm.hpp>
#include <bgfx/bgfx.h>
#include <cassert>

export module Utils;

import <vector>;
import <filesystem>;
import <fstream>;

/// <summary> Color structure for 4 byte colors </summary>
export class Color {
public:
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

// Make log work work color
export std::ostream& operator<<(std::ostream& target, const Color& c) {
    // uint8_t can't be <<'d
    target << '(' << (int)c.r << ", " << (int)c.g << ", " << (int)c.b << ", " << (int)c.a << ')';
    return target;
}
export std::ostream& operator<<(std::ostream& target, const uint8_t& c) {
    target << (int)c; // uint8_t can't be <<'d
    return target;
}

/// <summary> Various utility functions </summary>
export class Utils {
    // MSVC incorrectly complains about internal linkage when using namespaces so they're classes
    Utils() = delete;
public:

    /// <summary> Attempts to load a compiled bgfx shader, hard exits on fail </summary>
    static bgfx::ShaderHandle LoadShader(const std::filesystem::path& path) {
        auto buffer = LoadMemory(path);
        assert(buffer != nullptr);
        return bgfx::createShader(buffer);
    }

private:

    static const bgfx::Memory* LoadMemory(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) return nullptr;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        const bgfx::Memory* mem = bgfx::alloc(uint32_t(size + 1)); // Bgfx deals with freeing; leaks on read fail?
        if (file.read((char*)mem->data, size)) {
            mem->data[mem->size - 1] = '\0';
            return mem;
        }
        return nullptr;
    }
};

export namespace Globals {
    export const Color Red = Color(0xff, 0x00, 0x00, 0xff);
    export const Color Green = Color(0x00, 0xff, 0x00, 0xff);
    export const Color Blue = Color(0x00, 0x00, 0xff, 0xff);
    export const Color White = Color(0xff, 0xff, 0xff, 0xff);
    export const Color Black = Color(0x00, 0x00, 0x00, 0xff);
}