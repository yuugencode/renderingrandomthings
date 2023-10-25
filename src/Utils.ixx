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

    uint8_t r, g, b, a;

    Color() {} // r = g = b = a = 0x00; 
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        this->r = r; this->g = g; this->b = b; this->a = a;
    }

    static Color Lerp(const Color& a, const Color& b, float t) {
        Color c;
        c.r = (int)(a.r * (1.0f - t) + b.r * t);
        c.g = (int)(a.g * (1.0f - t) + b.g * t);
        c.b = (int)(a.b * (1.0f - t) + b.b * t);
        c.a = (int)(a.a * (1.0f - t) + b.a * t);
        return c;
    }

    Color Lerp(const Color& other, float t) {
        return Color::Lerp(*this, other, t);
    }

    // const static initializers don't play well with modules, these are statically assigned
    static Color Red;
};

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