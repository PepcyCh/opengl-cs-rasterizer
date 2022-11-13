#pragma once

#include "renderer.hpp"

class OctreeHiZRenderer final : public Renderer {
public:
    OctreeHiZRenderer(Rasterizer &rasterizer, const Scene &scene);

    void RenderScene() override;

    void DrawUi() override;

private:
    void ConstructOctree();

    void GenerateHiZ();

    std::unique_ptr<GlTexture2D> prev_depth_ = nullptr;
    std::unique_ptr<GlProgram> hiz_gen_program_ = nullptr;

    std::unique_ptr<GlBuffer> camera_info_buffer_ = nullptr;
    std::unique_ptr<GlBuffer> cull_result_buffer_ = nullptr;
    std::unique_ptr<GlProgram> bbox_cull_program_ = nullptr;
    std::unique_ptr<GlProgram> node_cull_program_ = nullptr;
    std::unique_ptr<GlProgram> init_buffer_program_ = nullptr;
    std::unique_ptr<GlProgram> calc_args_program_ = nullptr;

    struct OctreeNode {
        Bbox bbox;
        std::vector<uint32_t> instances;
        int ch[8];
        std::unique_ptr<GlBuffer> bbox_buffer = nullptr;
        uint32_t level;

        OctreeNode(Bbox bbox);

        bool IsLeaf() const { return ch[0] == -1; }
    };
    std::vector<OctreeNode> octree_nodes_;
    
    uint32_t octree_max_level_ = 0;
    std::unique_ptr<GlBuffer> dispatch_args_buffer_ = nullptr;
    std::unique_ptr<GlBuffer> octree_buffer_ = nullptr;
    std::unique_ptr<GlBuffer> io_nodes_buffer_[2];
    std::unique_ptr<GlBuffer> visible_nodes_buffer_ = nullptr;

    uint32_t num_drawn_instances_ = 0;
};
