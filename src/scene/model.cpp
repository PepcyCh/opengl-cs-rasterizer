#include "model.hpp"

#include <iostream>
#include <unordered_map>

#include <tiny_obj_loader.h>

namespace {

size_t HashCombine(size_t seed, size_t v) {
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

void FetchVertexData(const tinyobj::attrib_t &in_attribs, const tinyobj::index_t &index, bool has_normal,
    glm::vec3 &pos, glm::vec3 &norm) {
    pos[0] = in_attribs.vertices[3 * index.vertex_index];
    pos[1] = in_attribs.vertices[3 * index.vertex_index + 1];
    pos[2] = in_attribs.vertices[3 * index.vertex_index + 2];
    if (has_normal) {
        norm[0] = in_attribs.normals[3 * index.normal_index];
        norm[1] = in_attribs.normals[3 * index.normal_index + 1];
        norm[2] = in_attribs.normals[3 * index.normal_index + 2];
    } else {
        norm = glm::vec3(0.0f);
    }
}

}

bool operator==(const tinyobj::index_t &a, const tinyobj::index_t &b) noexcept {
    return a.vertex_index == b.vertex_index && a.normal_index == b.normal_index && a.texcoord_index == b.texcoord_index;
}
struct ObjIndex {
    tinyobj::index_t i;

    ObjIndex(const tinyobj::index_t &i) : i(i) {}

    bool operator==(const ObjIndex &rhs) const noexcept {
        return i == rhs.i;
    }
};
template <>
struct std::hash<ObjIndex> {
    size_t operator()(const ObjIndex &v) const noexcept {
        std::hash<int> hasher;
        size_t hash = hasher(v.i.vertex_index);
        hash = HashCombine(hash, hasher(v.i.normal_index));
        hash = HashCombine(hash, hasher(v.i.texcoord_index));
        return hash;
    }
};

Model::Model(const std::filesystem::path &obj_path) {
    tinyobj::attrib_t in_attribs;
    std::vector<tinyobj::shape_t> in_shapes;
    std::vector<tinyobj::material_t> in_materials;
    std::string load_warn;
    std::string load_err;
    bool result = tinyobj::LoadObj(&in_attribs, &in_shapes, &in_materials, &load_warn, &load_err,
        obj_path.string().c_str());
    if (!result) {
        std::cout << "Failed to load " << obj_path << "\n";
        return;
    }

    std::unordered_map<ObjIndex, size_t> index_map;
    const bool has_normal = !in_attribs.normals.empty();

    bbox_.Empty();
    for (auto &in_shape : in_shapes) {
        for (size_t f = 0; f < in_shape.mesh.indices.size() / 3; f++) {
            auto in_idx0 = in_shape.mesh.indices[3 * f];
            auto in_idx1 = in_shape.mesh.indices[3 * f + 1];
            auto in_idx2 = in_shape.mesh.indices[3 * f + 2];
            auto material_id = in_shape.mesh.material_ids[f];

            if (index_map.count(in_idx0) == 0) {
                index_map[in_idx0] = positions_.size();
                glm::vec3 pos, norm;
                FetchVertexData(in_attribs, in_idx0, has_normal, pos, norm);
                positions_.push_back(pos);
                normals_.push_back(norm);

                bbox_.Merge(pos);
            }
            indices_.push_back(index_map.at(in_idx0));

            if (index_map.count(in_idx1) == 0) {
                index_map[in_idx1] = positions_.size();
                glm::vec3 pos, norm;
                FetchVertexData(in_attribs, in_idx1, has_normal, pos, norm);
                positions_.push_back(pos);
                normals_.push_back(norm);

                bbox_.Merge(pos);
            }
            indices_.push_back(index_map.at(in_idx1));

            if (index_map.count(in_idx2) == 0) {
                index_map[in_idx2] = positions_.size();
                glm::vec3 pos, norm;
                FetchVertexData(in_attribs, in_idx2, has_normal, pos, norm);
                positions_.push_back(pos);
                normals_.push_back(norm);

                bbox_.Merge(pos);
            }
            indices_.push_back(index_map.at(in_idx2));
        }
    }

    if (!has_normal) {
        for (size_t i = 0; i < indices_.size(); i += 3) {
            glm::vec3 p0 = positions_[indices_[i]];
            glm::vec3 p1 = positions_[indices_[i + 1]];
            glm::vec3 p2 = positions_[indices_[i + 2]];
            glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
            normals_[indices_[i]] += n;
            normals_[indices_[i + 1]] += n;
            normals_[indices_[i + 2]] += n;
        }
        for (glm::vec3 &norm : normals_) {
            norm = glm::normalize(norm);
        }
    }
    
    size_t vertex_buffer_size = positions_.size() * sizeof(glm::vec3);
    position_buffer_ = std::make_unique<GlBuffer>(vertex_buffer_size, false, positions_.data());
    normal_buffer_ = std::make_unique<GlBuffer>(vertex_buffer_size, false, normals_.data());
    size_t index_buffer_size = indices_.size() * sizeof(uint32_t);
    index_buffer_ = std::make_unique<GlBuffer>(index_buffer_size, false, indices_.data());
}
