#pragma once

#include <filesystem>

#include <glm/glm.hpp>

#include "bbox.hpp"
#include "glh/resource.hpp"

class Model {
public:
    Model(const std::filesystem::path &obj_path);

    size_t VericesCount() const { return positions_.size(); }
    const std::vector<glm::vec3> &Positions() const { return positions_; }
    const std::vector<glm::vec3> &Normals() const { return normals_; }
    size_t IndicesCount() const { return index_.size(); }
    const std::vector<uint32_t> &Index() const { return index_; }

    const Bbox &Bbox() const { return bbox_; }

    const GlBuffer *PositionBuffer() const { return position_buffer_.get(); }
    const GlBuffer *NormalBuffer() const { return normal_buffer_.get(); }
    const GlBuffer *IndexBuffer() const { return index_buffer_.get(); }

private:
    std::vector<glm::vec3> positions_;
    std::vector<glm::vec3> normals_;
    std::vector<uint32_t> index_;
    struct Bbox bbox_;

    std::unique_ptr<GlBuffer> position_buffer_;
    std::unique_ptr<GlBuffer> normal_buffer_;
    std::unique_ptr<GlBuffer> index_buffer_;
};
