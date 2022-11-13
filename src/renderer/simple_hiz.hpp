#pragma once

#include "renderer.hpp"

class SimpleHiZRenderer final : public Renderer {
public:
    SimpleHiZRenderer(Rasterizer &rasterizer, const Scene &scene);

    void RenderScene() override;

    void DrawUi() override;

private:
    void GenerateHiZ();

    std::unique_ptr<GlProgram> fill_id_map_program_ = nullptr;
    std::unique_ptr<GlProgram> hiz_gen_program_ = nullptr;
    std::unique_ptr<GlProgram> hiz_cull_program_ = nullptr;

    std::unique_ptr<GlTexture2D> prev_depth_ = nullptr;

    std::unique_ptr<GlBuffer> bbox_buffer_ = nullptr;
    std::unique_ptr<GlBuffer> instances_id_map_buffer_ = nullptr;
    std::vector<uint32_t> output_instances_;
    std::unique_ptr<GlBuffer> output_instance_buffer_ = nullptr;
    std::unique_ptr<GlBuffer> cull_result_buffer_ = nullptr;
    std::unique_ptr<GlBuffer> camera_info_buffer_ = nullptr;

    uint32_t num_drawn_instances_ = 0;
};
