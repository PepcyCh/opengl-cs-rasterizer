#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "glh/program.hpp"
#include "glh/resource.hpp"

class Rasterizer {
public:
    Rasterizer(uint32_t width, uint32_t height);
    ~Rasterizer();

    void SetViewport(uint32_t width, uint32_t height);

    void SetColorTarget(const GlTexture2D *texture_);
    void SetDepthTarget(const GlTexture2D *texture_);
    const GlTexture2D *GetColorTarget() const { return frame_buffer_; }
    const GlTexture2D *GetDepthTarget() const { return depth_buffer_; }

    void SetClearColor(float r, float g, float b, float a = 1.0f);
    void SetClearDepth(float depth);
    void ClearBuffers();

    void SetMatrixProj(const glm::mat4 &proj);
    void SetMatrixView(const glm::mat4 &view);
    void SetMatrixModel(const glm::mat4 &model);
    glm::mat4 GetMatrixProj() const { return states_.proj; }
    glm::mat4 GetMatrixView() const { return states_.view; }

    void SetLightPosition(float x, float y, float z, float w);
    void SetLightEmission(float r, float g, float b);

    void SetPositionBuffer(const GlBuffer *buffer);
    void SetNormalBuffer(const GlBuffer *buffer);
    void SetIndexBuffer(const GlBuffer *buffer);

    void DrawIndexed(uint32_t num_indices, uint32_t first_index = 0, uint32_t vertex_offset = 0);

private:
    std::unique_ptr<GlProgram> rastertize_program_ = nullptr;
    std::unique_ptr<GlProgram> clear_program_ = nullptr;

    const GlTexture2D *frame_buffer_ = nullptr;
    const GlTexture2D *depth_buffer_ = nullptr;

    struct alignas(16) ClearValues {
        glm::vec4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
        float depth = 1.0f;
    } clear_values_;
    std::unique_ptr<GlBuffer> clear_values_buffer_ = nullptr;

    const GlBuffer *position_buffer_ = nullptr;
    const GlBuffer *normal_buffer_ = nullptr;
    const GlBuffer *index_buffer_ = nullptr;

    struct alignas(16) RasterizerStates {
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 model_it = glm::mat4(1.0f);
        glm::mat4 view;
        glm::mat4 proj;
        uint32_t viewport_width;
        uint32_t viewport_height;
    } states_;
    std::unique_ptr<GlBuffer> states_buffer_ = nullptr;

    struct alignas(16) DrawArguments {
        uint32_t num_indices;
        uint32_t first_index;
        uint32_t vertex_offset;
    } draw_args_;
    std::unique_ptr<GlBuffer> draw_args_buffer_ = nullptr;

    struct alignas(16) ShadingUniforms {
        glm::vec4 light_pos_dir = { 0.0f, 1.0f, 0.0f, 0.0f };
        glm::vec4 light_emission = { 1.0f, 1.0f, 1.0f, 1.0f };
    } shading_;
    std::unique_ptr<GlBuffer> shading_buffer_ = nullptr;

    std::unique_ptr<GlProgram> line_tile_pre_program_ = nullptr;
    std::unique_ptr<GlProgram> line_tile_draw_program_ = nullptr;
    std::unique_ptr<GlBuffer> out_vertices_buffer_ = nullptr;
    std::unique_ptr<GlBuffer> tile_list_num_buffer_ = nullptr;
    std::unique_ptr<GlBuffer> tile_list_buffer_ = nullptr;
};
