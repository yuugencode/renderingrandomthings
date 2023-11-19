#include "Utils.h"

#include "Engine/Log.h"

// Utils

bgfx::ShaderHandle Utils::LoadShader(const std::filesystem::path& path) {
    assert(std::filesystem::exists(path));

    std::ifstream file = std::ifstream(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    const bgfx::Memory* buffer = bgfx::alloc(uint32_t(size + 1));
    if (file.read((char*)buffer->data, size))
        buffer->data[buffer->size - 1] = '\0';

    assert(buffer != nullptr);
    return bgfx::createShader(buffer);
}

glm::vec3 Utils::Barycentric(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    const auto& v0 = b - a, v1 = c - a, v2 = p - a;
    const float d00 = glm::dot(v0, v0);
    const float d01 = glm::dot(v0, v1);
    const float d11 = glm::dot(v1, v1);
    const float d20 = glm::dot(v2, v0);
    const float d21 = glm::dot(v2, v1);
    const float denom = d00 * d11 - d01 * d01;
    const float v = (d11 * d20 - d01 * d21) / denom;
    const float w = (d00 * d21 - d01 * d20) / denom;
    const float u = 1.0f - v - w;
    return glm::vec3(u, v, w);
}

glm::vec3 Utils::ConfinedBarycentric(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    using namespace glm;

    const auto v0 = b - a, v1 = c - a, v2 = p - a;
    const float d00 = dot(v0, v0);
    const float d01 = dot(v0, v1);
    const float d11 = dot(v1, v1);
    const float d20 = dot(v2, v0);
    const float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    if (denom < 0.00001f) { // We're a line
        const float dist0 = dot(p - a, p - a);
        const float dist1 = dot(p - b, p - b);
        const float dist2 = dot(p - c, p - c);
        if (dist0 < dist1 && dist0 < dist2) return vec3(1.0f, 0.0f, 0.0f);
        else if (dist1 < dist2) return vec3(0.0f, 1.0f, 0.0f);
        else return vec3(0.0f, 0.0f, 1.0f);
    }
    const float v = (d11 * d20 - d01 * d21) / denom;
    const float w = (d00 * d21 - d01 * d20) / denom;
    const float u = 1.0f - v - w;
    
    if (u < 0.0f && v < 0.0f) // Behind z
        return vec3(0.0f, 0.0f, 1.0f);
    else if (u < 0.0f && w < 0.0f) // Behind y
        return vec3(0.0f, 1.0f, 0.0f);
    else if (v < 0.0f && w < 0.0f) // Behind x
        return vec3(1.0f, 0.0f, 0.0f);
    else if (u < 0.0f) { // Between y/z
        const auto sum = v + w;
        return vec3(0.0f, v / sum, w / sum);
    }
    else if (v < 0.0f) { // Between x/z
        const auto sum = u + w;
        return vec3(u / sum, 0.0f, w / sum);
    }
    else if (w < 0.0f) { // Between x/y
        const auto sum = u + v;
        return vec3(u / sum, v / sum, 0.0f);
    }

    return vec3(u, v, w); // Inside tri
}

inline float Utils::InvSegmentLerp(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b) {
    const auto pa = p - a;
    const auto ba = b - a;
    const auto baMagn = glm::length(ba);
    if (baMagn < 0.000001f) return 0.0f;
    const auto baNorm = ba / baMagn;
    const auto dot = glm::dot(pa, baNorm);
    if (dot < 0.0f) return 0.0f;
    const auto proj = baNorm * dot;
    const auto dist2 = baMagn;
    const auto dist3 = glm::length(proj);
    return glm::min(dist3 / dist2, 1.0f);
}

glm::vec4 Utils::InvQuadrilateral(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d) {
   
    // @TODO: This is a naive self written implementation
    // Probably tons of excess calculations and optimization available

    const float wt1 = InvSegmentLerp(p, a, b);
    const float wt2 = InvSegmentLerp(p, c, d);

    float wtA = 1.0f - wt1;
    float wtB = wt1;
    float wtC = 1.0f - wt2;
    float wtD = wt2;

    const float wt3 = InvSegmentLerp(p, a, c);
    const float wt4 = InvSegmentLerp(p, b, d);

    wtC *= wt3;
    wtA *= 1.0f - wt3;
    wtD *= wt4;
    wtB *= 1.0f - wt4;

    const float wt5 = InvSegmentLerp(p, a, d);
    const float wt6 = InvSegmentLerp(p, b, c);

    wtD *= wt5;
    wtA *= 1.0f - wt5;
    wtC *= wt6;
    wtB *= 1.0f - wt6;

    const float sum = wtA + wtB + wtC + wtD;

    if (sum < 0.001f) return glm::vec4(0.0f);

    return glm::vec4(wtA, wtB, wtC, wtD) / sum;
}

glm::vec3 Utils::Project(const glm::vec3& a, const glm::vec3& b) {
    return b * glm::dot(a, b);
}

glm::vec3 Utils::ProjectOnPlane(const glm::vec3& vec, const glm::vec3& normal) {
    return vec - Project(vec, normal);
}

float Utils::InvLerp(const float& val, const float& min, const float& max) {
    return (val - min) / (max - min);
}

float Utils::InvLerpClamp(const float& val, const float& min, const float& max) {
    return InvLerp(glm::clamp(val, min, max), min, max);
}

float Utils::SqrLength(const glm::vec3& a) {
    return glm::dot(a, a);
}

glm::vec3 Utils::GenRandomVec(const glm::vec3& dir, const float& hashA, const float& hashB) {
    auto temp1 = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::dot(temp1, temp1) < 0.0001f) temp1 = glm::cross(dir, glm::vec3(1.0f, 0.0f, 0.0f));
    const auto temp2 = glm::cross(dir, temp1);
    return glm::normalize(dir + temp1 * hashA + temp2 * hashB);
}

// Nice hash functions from https://www.shadertoy.com/view/4djSRW
float Utils::Hash11(float p) {
    p = glm::fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return glm::fract(p);
}
float Utils::Hash13(glm::vec3 p3) {
    p3 = glm::fract(p3 * 0.1031f);
    p3 += glm::dot(p3, glm::vec3(p3.y, p3.z, p3.x) + 33.33f);
    return glm::fract((p3.x + p3.y) * p3.z);
}
glm::vec3 Utils::Hash32(const glm::vec2& p) {
    glm::vec3 p3 = glm::fract(glm::vec3(p.x, p.y, p.x) * glm::vec3(0.1031f, 0.1030f, 0.0973f));
    p3 += glm::dot(p3, glm::vec3(p3.y, p3.x, p3.z) + 33.33f);
    return glm::fract((glm::vec3(p3.x, p3.x, p3.y) + glm::vec3(p3.y, p3.z, p3.z)) * glm::vec3(p3.z, p3.y, p3.x));
}
glm::vec2 Utils::Hash22(const glm::vec2& p) {
    glm::vec3 p3 = glm::fract(glm::vec3(p.x, p.y, p.x) * glm::vec3(0.1031f, 0.1030f, 0.0973f));
    p3 += glm::dot(p3, glm::vec3(p3.y, p3.z, p3.x) + 33.33f);
    return glm::fract((glm::vec2(p3.x, p3.x) + glm::vec2(p3.y, p3.z)) * glm::vec2(p3.z, p3.y));
}
glm::vec2 Utils::Hash23(glm::vec3 p3) {
    p3 = glm::fract(p3 * glm::vec3(0.1031f, 0.1030f, 0.0973f));
    p3 += glm::dot(p3, glm::vec3(p3.y, p3.z, p3.x) + 33.33f);
    return glm::fract((glm::vec2(p3.x, p3.x) + glm::vec2(p3.y, p3.z)) * glm::vec2(p3.z, p3.y));
}


glm::mat4x4 Utils::ModelMatrix(const glm::vec3& pos, const glm::vec3& lookAtTarget, const glm::vec3& scale) {
    auto mat_pos = glm::translate(glm::mat4x4(1.0f), pos);
    auto mat_rot = glm::mat4x4(glm::quatLookAt(glm::normalize(lookAtTarget - pos), glm::vec3(0, 1, 0)));
    auto mat_scl = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    return mat_pos * mat_rot * mat_scl;
}

template <typename T> requires std::is_enum_v<T>
bool Utils::HasFlag(const T& val, const T& flag) {
    return (val & flag) != 0;
}

void Utils::PrintMatrix(const glm::mat4x4& matrix) {
    Log::Line("Matrix4x4:");
    Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][0]), Log::FormatFloat(matrix[1][0]), Log::FormatFloat(matrix[2][0]), Log::FormatFloat(matrix[3][0]));
    Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][1]), Log::FormatFloat(matrix[1][1]), Log::FormatFloat(matrix[2][1]), Log::FormatFloat(matrix[3][1]));
    Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][2]), Log::FormatFloat(matrix[1][2]), Log::FormatFloat(matrix[2][2]), Log::FormatFloat(matrix[3][2]));
    Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][3]), Log::FormatFloat(matrix[1][3]), Log::FormatFloat(matrix[2][3]), Log::FormatFloat(matrix[3][3]));
}
