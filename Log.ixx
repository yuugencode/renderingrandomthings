module;

#include <bgfx/bgfx.h>

export module Log;

import <string>;
import <string_view>;
import <format>;
import <iostream>;

/// <summary> Contains various utility logging functions </summary>
export class Log {
public:

    /// <summary> Simple print, eats almost anything </summary>
    template <typename... Args>
    static void Line(const Args&... args) {
        ([&] { std::cout << args << std::endl; } (), ...);
    }
    
    /// <summary> Preformatted variable length print </summary>
    template <typename... Args>
    static void Formatted(std::string_view str, const Args&... args) {
        std::cout << std::vformat(str, std::make_format_args(args...)) << std::endl;
    }

    /// <summary> Prints text on screen for 1 frame </summary>
    static void Screen(uint16_t line, const char* str) {
        bgfx::dbgTextPrintf(0, line, 0x0f, str);
    }

    /// <summary> Prints text on screen for 1 frame </summary>
    template <typename... Args>
    static void Screen(uint16_t line, std::string_view str, const Args&... args) {
        // Make into null terminated string
        auto temp = std::string(std::vformat(str, std::make_format_args(args...)));
        Log::Screen(line, temp.c_str());
    }
};