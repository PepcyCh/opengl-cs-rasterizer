#include "bbox.hpp"

glm::vec3 Bbox::Centroid() const {
    return (pmin + pmax) * 0.5f;
}

float Bbox::Extent() const {
    return glm::length((pmax - pmin));
}

void Bbox::Empty() {
    constexpr float float_max = std::numeric_limits<float>::max();
    constexpr float float_min = std::numeric_limits<float>::lowest();
    pmin = glm::vec3(float_max, float_max, float_max);
    pmax = glm::vec3(float_min, float_min, float_min);
}

Bbox Bbox::TransformBy(const glm::mat4 &mat) const {
    glm::vec3 p0 = mat * glm::vec4(pmin, 1.0f);
    glm::vec3 p1 = mat * glm::vec4(pmax.x, pmin.y, pmin.z, 1.0f);
    glm::vec3 p2 = mat * glm::vec4(pmin.x, pmax.y, pmin.z, 1.0f);
    glm::vec3 p3 = mat * glm::vec4(pmax.x, pmax.y, pmin.z, 1.0f);
    glm::vec3 p4 = mat * glm::vec4(pmin.x, pmin.y, pmax.z, 1.0f);
    glm::vec3 p5 = mat * glm::vec4(pmax.x, pmin.y, pmax.z, 1.0f);
    glm::vec3 p6 = mat * glm::vec4(pmin.x, pmax.y, pmax.z, 1.0f);
    glm::vec3 p7 = mat * glm::vec4(pmax, 1.0f);
    return Bbox {
        .pmin = glm::min(
            glm::min(glm::min(p0, p1), glm::min(p2, p3)),
            glm::min(glm::min(p4, p5), glm::min(p6, p7))
        ),
        .pmax = glm::max(
            glm::max(glm::max(p0, p1), glm::max(p2, p3)),
            glm::max(glm::max(p4, p5), glm::max(p6, p7))
        ),
    };
}

void Bbox::Merge(const Bbox &rhs) {
    pmin = glm::min(pmin, rhs.pmin);
    pmax = glm::max(pmax, rhs.pmax);
}

void Bbox::Merge(const glm::vec3 &p) {
    pmin = glm::min(pmin, p);
    pmax = glm::max(pmax, p);
}

bool Bbox::IntersectWith(const Bbox &rhs) const {
    return pmin.x <= rhs.pmax.x && pmax.x >= rhs.pmin.x
        && pmin.y <= rhs.pmax.y && pmax.y >= rhs.pmin.y
        && pmin.z <= rhs.pmax.z && pmax.z >= rhs.pmin.z;
}
