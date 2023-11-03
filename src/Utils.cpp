#include "Utils.h"

#include "Log.h"

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

glm::mat4x4 Utils::ModelMatrix(const glm::vec3& pos, const glm::vec3& lookAtTarget, const glm::vec3& scale) {
    auto mat_pos = glm::translate(glm::mat4x4(1.0f), pos);
    auto mat_rot = glm::mat4x4(glm::quatLookAt(glm::normalize(lookAtTarget - pos), glm::vec3(0, 1, 0)));
    auto mat_scl = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    return mat_pos * mat_rot * mat_scl;
}

void Utils::PrintMatrix(const glm::mat4x4& matrix) {
    Log::Line("Matrix4x4:");
    Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][0]), Log::FormatFloat(matrix[1][0]), Log::FormatFloat(matrix[2][0]), Log::FormatFloat(matrix[3][0]));
    Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][1]), Log::FormatFloat(matrix[1][1]), Log::FormatFloat(matrix[2][1]), Log::FormatFloat(matrix[3][1]));
    Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][2]), Log::FormatFloat(matrix[1][2]), Log::FormatFloat(matrix[2][2]), Log::FormatFloat(matrix[3][2]));
    Log::LineFormatted("({}, {}, {}, {})", Log::FormatFloat(matrix[0][3]), Log::FormatFloat(matrix[1][3]), Log::FormatFloat(matrix[2][3]), Log::FormatFloat(matrix[3][3]));
}
