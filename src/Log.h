#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <format>

#include "bgfx/bgfx.h"

// Contains various utility logging functions 
namespace Log {

    // Simple print, eats almost anything 
    template <typename... Args>
    inline void Line(const Args&... args) {
        const std::size_t num = sizeof...(Args);
        int i = 0;
        ([&] { std::cout << args; if ((++i) != num) std::cout << ", "; } (), ...);
        std::cout << std::endl;
    }

    // Prints an error and hard exits the program 
    template <typename... Args>
    inline void FatalError(const Args&... args) {
        Line(args...);
        abort();
    }

    // Preformatted variable length print 
    template <typename... Args>
    inline void LineFormatted(std::string_view str, const Args&... args) {
        std::cout << std::vformat(str, std::make_format_args(args...)) << std::endl;
    }

    // Prints text on screen for 1 frame 
    inline void Screen(uint16_t line, const char* str) {
        bgfx::dbgTextPrintf(0, line, 0x0f, str);
    }

    // Prints text on screen for 1 frame 
    template <typename... Args>
    inline void Screen(uint16_t line, std::string_view str, const Args&... args) {
        // Make into null terminated string
        auto temp = std::string(std::vformat(str, std::make_format_args(args...)));
        Log::Screen(line, temp.c_str());
    }

    // Hacky way to print floats with 2 decimals so that indentation stays despite sign changing 
    inline std::string FormatFloat(const float& val) {
        std::string ret = std::format("{:.2f}", val);
        if (ret.size() > 4) return ret;
        else return " " + ret;
    }
};
