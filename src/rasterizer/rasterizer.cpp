#include "rasterizer.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>

#include <glad/glad.h>

#include "utils.hpp"

namespace {

constexpr uint32_t kComputeWorkGroupSize = 32;

constexpr uint32_t kMaxTrianglesPerList = 1024;

struct Vertex {
    glm::vec3 pos_world;
    float screen_x;
    glm::vec3 normal_world;
    float screen_y;
    glm::vec4 homo;
    glm::vec3 clip;
    float inv_w;
};

struct ListTriangle {
    uint32_t min_x;
    uint32_t max_x;
    uint32_t tri_index;
    float inv_area;
};

}

Rasterizer::Rasterizer(uint32_t width, uint32_t height) {
    CreateComputeProgram(rastertize_program_, kShaderSourceDir / "rasterizer/scanline.comp");
    CreateComputeProgram(clear_program_, kShaderSourceDir / "rasterizer/clear.comp");

    clear_values_buffer_ = std::make_unique<GlBuffer>(sizeof(ClearValues), GL_DYNAMIC_STORAGE_BIT);
    states_buffer_ = std::make_unique<GlBuffer>(sizeof(RasterizerStates), GL_DYNAMIC_STORAGE_BIT);
    draw_args_buffer_ = std::make_unique<GlBuffer>(sizeof(DrawArguments), GL_DYNAMIC_STORAGE_BIT);
    shading_buffer_ = std::make_unique<GlBuffer>(sizeof(ShadingUniforms), GL_DYNAMIC_STORAGE_BIT);

    CreateComputeProgram(line_tile_pre_program_, kShaderSourceDir / "rasterizer/line_tile_pre.comp");
    CreateComputeProgram(line_tile_draw_program_, kShaderSourceDir / "rasterizer/line_tile_draw.comp");
}

Rasterizer::~Rasterizer() {}

void Rasterizer::SetViewport(uint32_t width, uint32_t height) {
    states_.viewport_width = width;
    states_.viewport_height = height;

    std::vector<uint32_t> zeros(height, 0);
    tile_list_num_buffer_ = std::make_unique<GlBuffer>(height * sizeof(uint32_t), 0, zeros.data());
    tile_list_buffer_ = std::make_unique<GlBuffer>(height * kMaxTrianglesPerList * sizeof(ListTriangle));
}

void Rasterizer::SetColorTarget(const GlTexture2D *texture_) {
    frame_buffer_ = texture_;
}

void Rasterizer::SetDepthTarget(const GlTexture2D *texture_) {
    depth_buffer_ = texture_;
}

void Rasterizer::SetClearColor(float r, float g, float b, float a) {
    clear_values_.color = { r, g, b, a };
}

void Rasterizer::SetClearDepth(float depth) {
    clear_values_.depth = depth;
}

void Rasterizer::ClearBuffers() {
    glNamedBufferSubData(clear_values_buffer_->Id(), 0, sizeof(ClearValues), &clear_values_);

    glUseProgram(clear_program_->Id());

    glBindImageTexture(0, frame_buffer_->Id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, frame_buffer_->Format());
    glBindImageTexture(1, depth_buffer_->Id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, depth_buffer_->Format());
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, clear_values_buffer_->Id());

    glDispatchCompute((states_.viewport_width + 15) / 16, (states_.viewport_height + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glUseProgram(0);
}

void Rasterizer::SetMatrixProj(const glm::mat4 &proj) {
    states_.proj = proj;
}

void Rasterizer::SetMatrixView(const glm::mat4 &view) {
    states_.view = view;
}

void Rasterizer::SetMatrixModel(const glm::mat4 &model) {
    states_.model = model;
    states_.model_it = glm::transpose(glm::inverse(model));
}

void Rasterizer::SetLightPosition(float x, float y, float z, float w) {
    shading_.light_pos_dir = { x, y, z, w };
}

void Rasterizer::SetLightEmission(float r, float g, float b) {
    shading_.light_emission = { r, g, b, 1.0f };
}

void Rasterizer::SetPositionBuffer(const GlBuffer *buffer) {
    position_buffer_ = buffer;
}

void Rasterizer::SetNormalBuffer(const GlBuffer *buffer) {
    normal_buffer_ = buffer;
}

void Rasterizer::SetIndexBuffer(const GlBuffer *buffer) {
    index_buffer_ = buffer;
}

void Rasterizer::DrawIndexed(uint32_t num_indices, uint32_t first_index, uint32_t vertex_offset) {
    draw_args_.num_indices = num_indices;
    draw_args_.first_index = first_index;
    draw_args_.vertex_offset = vertex_offset;
    glNamedBufferSubData(states_buffer_->Id(), 0, sizeof(RasterizerStates), &states_);
    glNamedBufferSubData(draw_args_buffer_->Id(), 0, sizeof(DrawArguments), &draw_args_);
    glNamedBufferSubData(shading_buffer_->Id(), 0, sizeof(ShadingUniforms), &shading_);

#if 0
    glUseProgram(rastertize_program_->Id());

    uint32_t storage_buffers[] = {
        position_buffer_->Id(),
        normal_buffer_->Id(),
        index_buffer_->Id(),
    };
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 3, storage_buffers);

    glBindImageTexture(3, frame_buffer_->Id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, frame_buffer_->Format());
    glBindImageTexture(4, depth_buffer_->Id(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);

    uint32_t uniform_buffers[] = {
        states_buffer_->Id(),
        draw_args_buffer_->Id(),
        shading_buffer_->Id(),
    };
    glBindBuffersBase(GL_UNIFORM_BUFFER, 5, 3, uniform_buffers);

    glDispatchCompute((num_indices / 3 + kComputeWorkGroupSize - 1) / kComputeWorkGroupSize, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    glUseProgram(0);
#else
    auto vertices_buffer_size = num_indices * sizeof(Vertex);
    if (out_vertices_buffer_ == nullptr || out_vertices_buffer_->Size() < vertices_buffer_size) {
        out_vertices_buffer_ = std::make_unique<GlBuffer>(vertices_buffer_size);
    }

    glUseProgram(line_tile_pre_program_->Id());

    std::vector<uint32_t> storage_buffers {
        position_buffer_->Id(),
        normal_buffer_->Id(),
        index_buffer_->Id(),
        out_vertices_buffer_->Id(),
        tile_list_num_buffer_->Id(),
        tile_list_buffer_->Id(),
    };
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 6, storage_buffers.data());

    std::vector<uint32_t> uniform_buffers {
        states_buffer_->Id(),
        draw_args_buffer_->Id(),
    };
    glBindBuffersBase(GL_UNIFORM_BUFFER, 6, 2, uniform_buffers.data());

    glDispatchCompute((num_indices / 3 + kComputeWorkGroupSize - 1) / kComputeWorkGroupSize, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(line_tile_draw_program_->Id());

    storage_buffers = {
        out_vertices_buffer_->Id(),
        tile_list_num_buffer_->Id(),
        tile_list_buffer_->Id(),
    };
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 3, storage_buffers.data());

    glBindImageTexture(3, frame_buffer_->Id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, frame_buffer_->Format());
    glBindImageTexture(4, depth_buffer_->Id(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);

    uniform_buffers = {
        states_buffer_->Id(),
        shading_buffer_->Id(),
    };
    glBindBuffersBase(GL_UNIFORM_BUFFER, 5, 2, uniform_buffers.data());

    glDispatchCompute((states_.viewport_height + kComputeWorkGroupSize - 1) / kComputeWorkGroupSize, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

    glUseProgram(0);
#endif
}
