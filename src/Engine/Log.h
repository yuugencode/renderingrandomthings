#pragma once

#include <fmt/core.h>
#include <bgfx/bgfx.h>

// Logs a message and hard exits the program immediately
#define LOG_FATAL_AND_EXIT(x) { fmt::println(x); abort(); }

// Logs a formatted message and hard exits the program immediately
#define LOG_FATAL_AND_EXIT_ARG(x, y) { fmt::println(x, y); abort(); }

// Contains various utility logging functions 
namespace Log {

    // Prints text on screen for 1 frame 
    inline void Screen(uint16_t line, const char* str) {
        bgfx::dbgTextPrintf(0, line, 0x0f, str);
    }

    // Prints text on screen for 1 frame 
    template <typename... Args>
    inline void Screen(uint16_t line, std::string_view str, const Args&... args) {
        // Make into null terminated string
        auto temp = fmt::format(str, fmt::make_format_args(args...));
        Log::Screen(line, temp.c_str());
    }

    // Hacky way to print floats with 2 decimals so that indentation stays the same despite sign changing 
    inline std::string FormatFloat(float val) {
        // Sign comparison with <, > can get weird undefined results with negative zero floats so just compare string length
        std::string ret = fmt::format("{:.2f}", val);
        if (ret.size() > 4) return ret;
        else return " " + ret;
    }
};
