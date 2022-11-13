#pragma once

#include <glm/glm.hpp>

struct Bbox {
    glm::vec3 pmin;
    glm::vec3 pmax;

    void Empty();

    glm::vec3 Centroid() const;
    float Extent() const;

    Bbox TransformBy(const glm::mat4 &mat) const;
    void Merge(const Bbox &rhs);
    void Merge(const glm::vec3 &p);

    bool IntersectWith(const Bbox &rhs) const;
};
