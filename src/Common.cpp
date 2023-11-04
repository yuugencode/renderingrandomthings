#include "Common.h"

// Color

Color Color::FromFloat(const float& r, const float& g, const float& b, const float& a) {
    Color c;
    c.r = static_cast<uint8_t>(glm::clamp(r * 255.0f, 0.0f, 255.0f));
    c.g = static_cast<uint8_t>(glm::clamp(g * 255.0f, 0.0f, 255.0f));
    c.b = static_cast<uint8_t>(glm::clamp(b * 255.0f, 0.0f, 255.0f));
    c.a = static_cast<uint8_t>(glm::clamp(a * 255.0f, 0.0f, 255.0f));
    return c;
}
Color Color::FromFloat(const float& x) {
    Color c;
    c.r = static_cast<uint8_t>(glm::clamp(x * 255.0f, 0.0f, 255.0f));
    c.g = static_cast<uint8_t>(glm::clamp(x * 255.0f, 0.0f, 255.0f));
    c.b = static_cast<uint8_t>(glm::clamp(x * 255.0f, 0.0f, 255.0f));
    c.a = 255;
    return c;
}

Color Color::FromVec(const glm::vec3& v, const float& alpha) {
    Color c;
    c.r = static_cast<uint8_t>(glm::clamp(v.x * 255.0f, 0.0f, 255.0f));
    c.g = static_cast<uint8_t>(glm::clamp(v.y * 255.0f, 0.0f, 255.0f));
    c.b = static_cast<uint8_t>(glm::clamp(v.z * 255.0f, 0.0f, 255.0f));
    c.a = static_cast<uint8_t>(alpha);
    return c;
}

Color Color::FromVec(const glm::vec4& v) {
    Color c;
    c.r = static_cast<uint8_t>(glm::clamp(v.x * 255.0f, 0.0f, 255.0f));
    c.g = static_cast<uint8_t>(glm::clamp(v.y * 255.0f, 0.0f, 255.0f));
    c.b = static_cast<uint8_t>(glm::clamp(v.z * 255.0f, 0.0f, 255.0f));
    c.a = static_cast<uint8_t>(glm::clamp(v.w * 255.0f, 0.0f, 255.0f));
    return c;
}

Color Color::Lerp(const Color& a, const Color& b, const float& t) {
    Color c;
    c.r = (uint8_t)((float)a.r * (1.0f - t) + (float)b.r * t);
    c.g = (uint8_t)((float)a.g * (1.0f - t) + (float)b.g * t);
    c.b = (uint8_t)((float)a.b * (1.0f - t) + (float)b.b * t);
    c.a = (uint8_t)((float)a.a * (1.0f - t) + (float)b.a * t);
    return c;
}

Color Color::Lerp(const Color& other, const float& t) const {
    return Color::Lerp(*this, other, t);
}

glm::vec4 Color::ToVec4() const {
    return glm::vec4(r, g, b, a) / 255.0f;
}

Color Color::operator+(const Color& c) {
    Color ret;
    ret.r = r + c.r; ret.g = r + c.g; ret.b = r + c.b; ret.a = r + c.a;
    return ret;
}

Color& Color::operator+=(const Color& rhs) {
    r += rhs.r; g += rhs.g; b += rhs.b; a += rhs.a;
    return *this;
}


// AABB

AABB::AABB() { min = max = glm::vec3(0); }
AABB::AABB(const float& size) { min = glm::vec3(-size); max = glm::vec3(size); }
AABB::AABB(const glm::vec3& min, const glm::vec3& max) { this->min = min; this->max = max; }
AABB::AABB(const glm::vec3& pos) { this->min = pos; this->max = pos; }
AABB::AABB(const glm::vec3& pos, const float& radius) { this->min = pos - radius; this->max = pos + radius; }

glm::vec3 AABB::Center() const { return max * 0.5f + min * 0.5f; }

glm::vec3 AABB::Size() const { return max - min; }

bool AABB::Contains(const glm::vec3& pt) const {
    return pt.x >= min.x && pt.x <= max.x &&
        pt.y >= min.y && pt.y <= max.y &&
        pt.z >= min.z && pt.z <= max.z;
}

void AABB::Encapsulate(const glm::vec3& point) {
    min = glm::min(point, min);
    max = glm::max(point, max);
}

float AABB::AreaHeuristic() const {
    const auto temp = max - min;
    return temp.x + temp.y + temp.z;
}

// Standard slab method
float AABB::Intersect(const Ray& ray) const {
    const auto a = (min - ray.ro) * ray.inv_rd;
    const auto b = (max - ray.ro) * ray.inv_rd;
    const auto mi = glm::min(a, b);
    const auto ma = glm::max(a, b);
    const auto minT = glm::max(glm::max(mi.x, mi.y), mi.z);
    const auto maxT = glm::min(glm::min(ma.x, ma.y), ma.z);
    if (minT > maxT) return 0.0f;
    if (maxT < 0.0f) return maxT;
    return minT;
}