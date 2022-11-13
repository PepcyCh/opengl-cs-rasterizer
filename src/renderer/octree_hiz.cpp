#include "octree_hiz.hpp"

#include <algorithm>
#include <stack>

#include <glad/glad.h>
#include <imgui.h>

#include "rasterizer/utils.hpp"

namespace {

constexpr uint32_t kOctreeLeafSize = 1;
constexpr float kOctreeLeafExtent = 1.0f;

struct CameraInfo {
    glm::mat4 view;
    glm::mat4 proj;
};

}

OctreeHiZRenderer::OctreeHiZRenderer(Rasterizer &rasterizer, const Scene &scene) : Renderer(rasterizer, scene) {
    ConstructOctree();

    CreateComputeProgram(hiz_gen_program_, kShaderSourceDir / "hiz_gen.comp");
    CreateComputeProgram(bbox_cull_program_, kShaderSourceDir / "octree_hiz/bbox_test.comp");
    CreateComputeProgram(node_cull_program_, kShaderSourceDir / "octree_hiz/node_test.comp");
    CreateComputeProgram(init_buffer_program_, kShaderSourceDir / "octree_hiz/init_buffer.comp");
    CreateComputeProgram(calc_args_program_, kShaderSourceDir / "octree_hiz/calc_args.comp");

    camera_info_buffer_ = std::make_unique<GlBuffer>(sizeof(CameraInfo), GL_DYNAMIC_STORAGE_BIT);
    cull_result_buffer_ = std::make_unique<GlBuffer>(sizeof(uint32_t) * 8, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

    dispatch_args_buffer_ = std::make_unique<GlBuffer>(sizeof(uint32_t) * 3);
}

void OctreeHiZRenderer::RenderScene() {
    auto depth_buffer = rasterizer_.GetDepthTarget();
    bool can_do_cull = true;
    if (!prev_depth_ || prev_depth_->Width() != depth_buffer->Width()
        || prev_depth_->Height() != depth_buffer->Height()) {
        prev_depth_ = std::make_unique<GlTexture2D>(depth_buffer->Format(), depth_buffer->Width(),
            depth_buffer->Height(), 0);
        can_do_cull = false;
    }

    if (!can_do_cull) {
        scene_.ForEachInstance([this](const Scene::Instance &inst, const Model &model) {
            rasterizer_.SetMatrixModel(inst.transform);
            rasterizer_.SetPositionBuffer(model.PositionBuffer());
            rasterizer_.SetNormalBuffer(model.NormalBuffer());
            rasterizer_.SetIndexBuffer(model.IndexBuffer());
            rasterizer_.DrawIndexed(model.IndicesCount());
        });
        GenerateHiZ();
        return;
    }

    CameraInfo camera {
        .view = rasterizer_.GetMatrixView(),
        .proj = rasterizer_.GetMatrixProj(),
    };
    glNamedBufferSubData(camera_info_buffer_->Id(), 0, sizeof(CameraInfo), &camera);

#if 0
    std::vector<bool> drawn_flags(scene_.InstancesCount(), false);
    num_drawn_instances_ = 0;

    std::stack<const OctreeNode *> stack;
    stack.push(&octree_nodes_[0]);
    while (!stack.empty()) {
        auto u = stack.top();
        stack.pop();

        if (u->IsLeaf()) {
            for (auto inst_id : u->instances) {
                if (!drawn_flags[inst_id]) {
                    const auto &inst = scene_.GetInstance(inst_id);
                    const auto &model = scene_.GetModel(inst.model);
                    rasterizer_.SetMatrixModel(inst.transform);
                    rasterizer_.SetPositionBuffer(model.PositionBuffer());
                    rasterizer_.SetNormalBuffer(model.NormalBuffer());
                    rasterizer_.SetIndexBuffer(model.IndexBuffer());
                    rasterizer_.DrawIndexed(model.IndicesCount());
                    drawn_flags[inst_id] = true;
                    ++num_drawn_instances_;
                }
            }
            continue;
        }

        auto p_visible = cull_result_buffer_->TypedMap<uint32_t>();
        std::fill_n(p_visible, 8, 0);
        cull_result_buffer_->Unmap();

        glUseProgram(bbox_cull_program_->Id());
        glBindTextureUnit(0, prev_depth_->Id());
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, u->bbox_buffer->Id());
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, camera_info_buffer_->Id());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cull_result_buffer_->Id());
        glDispatchCompute(1, 1, 1);
        glUseProgram(0);

        p_visible = cull_result_buffer_->TypedMap<uint32_t>();

        for (uint32_t i = 0; i < 8; i++) {
            if (p_visible[i] != 0) {
                stack.push(&octree_nodes_[u->ch[i]]);
            }
        }

        cull_result_buffer_->Unmap();
    }
#else
    size_t curr_in_buffer = 0;

    glUseProgram(init_buffer_program_->Id());
    uint32_t init_buffers[] = {
        io_nodes_buffer_[curr_in_buffer]->Id(),
        visible_nodes_buffer_->Id(),
    };
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 2, init_buffers);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dispatch_args_buffer_->Id());
    for (uint32_t i = 0; i <= octree_max_level_; i++) {
        glUseProgram(calc_args_program_->Id());
        uint32_t calc_args_buffers[] = {
            io_nodes_buffer_[curr_in_buffer]->Id(),
            io_nodes_buffer_[curr_in_buffer ^ 1]->Id(),
            dispatch_args_buffer_->Id(),
        };
        glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 3, calc_args_buffers);
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

        glUseProgram(node_cull_program_->Id());
        glBindTextureUnit(0, prev_depth_->Id());
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, camera_info_buffer_->Id());
        uint32_t cull_buffers[] = {
            octree_buffer_->Id(),
            io_nodes_buffer_[curr_in_buffer]->Id(),
            io_nodes_buffer_[curr_in_buffer ^ 1]->Id(),
            visible_nodes_buffer_->Id(),
        };
        glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 2, 4, cull_buffers);
        glDispatchComputeIndirect(0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        
        curr_in_buffer ^= 1;
    }
    glUseProgram(0);
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);

    auto p_visible_buffer = visible_nodes_buffer_->TypedMap<uint32_t>();
    std::vector<uint32_t> visible_nodes(p_visible_buffer[0]);
    std::copy_n(p_visible_buffer + 1, visible_nodes.size(), visible_nodes.data());
    visible_nodes_buffer_->Unmap();

    std::vector<bool> drawn_flags(scene_.InstancesCount(), false);
    num_drawn_instances_ = 0;
    for (auto node_id : visible_nodes) {
        for (auto inst_id : octree_nodes_[node_id].instances) {
            if (!drawn_flags[inst_id]) {
                const auto &inst = scene_.GetInstance(inst_id);
                const auto &model = scene_.GetModel(inst.model);
                rasterizer_.SetMatrixModel(inst.transform);
                rasterizer_.SetPositionBuffer(model.PositionBuffer());
                rasterizer_.SetNormalBuffer(model.NormalBuffer());
                rasterizer_.SetIndexBuffer(model.IndexBuffer());
                rasterizer_.DrawIndexed(model.IndicesCount());
                drawn_flags[inst_id] = true;
                ++num_drawn_instances_;
            }
        }
    }
#endif

    GenerateHiZ();
}

void OctreeHiZRenderer::DrawUi() {
    ImGui::Text("Culling: %d / %d", num_drawn_instances_, static_cast<uint32_t>(scene_.InstancesCount()));
}

void OctreeHiZRenderer::ConstructOctree() {
    OctreeNode root(scene_.Bbox());
    root.instances.resize(scene_.InstancesCount());
    for (size_t i = 0; i < scene_.InstancesCount(); i++) {
        root.instances[i] = i;
    }
    octree_nodes_.push_back(std::move(root));
    octree_max_level_ = 0;

    std::stack<uint32_t> stack;
    stack.push(0);
    while (!stack.empty()) {
        auto u = stack.top();
        stack.pop();

        if (octree_nodes_[u].instances.size() <= kOctreeLeafSize
            || octree_nodes_[u].bbox.Extent() <= kOctreeLeafExtent) {
            continue;
        }

        auto u_pmin = octree_nodes_[u].bbox.pmin;
        auto delta = (octree_nodes_[u].bbox.pmax - octree_nodes_[u].bbox.pmin) * 0.5f;
        Bbox bboxes[8];
        for (size_t i = 0; i < 8; i++) {
            auto c_pmin = u_pmin + glm::vec3(
                (i & 1) == 0 ? 0.0 : delta.x,
                (i & 2) == 0 ? 0.0 : delta.y,
                (i & 4) == 0 ? 0.0 : delta.z
            );
            Bbox c_bbox { c_pmin, c_pmin + delta };
            OctreeNode c(c_bbox);
            c.level = octree_nodes_[u].level + 1;
            octree_max_level_ = std::max(octree_max_level_, c.level);
            for (auto inst_id : octree_nodes_[u].instances) {
                const auto &inst_bbox = scene_.GetInstance(inst_id).bbox;
                if (c.bbox.IntersectWith(inst_bbox)) {
                    c.instances.push_back(inst_id);
                }
            }
            uint32_t c_id = octree_nodes_.size();
            octree_nodes_[u].ch[i] = c_id;
            octree_nodes_.push_back(std::move(c));
            stack.push(c_id);
            bboxes[i] = c_bbox;
        }

        octree_nodes_[u].bbox_buffer = std::make_unique<GlBuffer>(sizeof(bboxes), 0, bboxes);
    }

    struct GpuOctreeNode {
        int ch[8];
        Bbox bbox;
        float _pad0;
        float _pad1;
    };
    std::vector<GpuOctreeNode> gpu_octree(octree_nodes_.size());
    for (size_t i = 0; i < octree_nodes_.size(); i++) {
        gpu_octree[i].bbox = octree_nodes_[i].bbox;
        std::copy_n(octree_nodes_[i].ch, 8, gpu_octree[i].ch);
    }
    octree_buffer_ = std::make_unique<GlBuffer>(octree_nodes_.size() * sizeof(GpuOctreeNode), 0, gpu_octree.data());
    auto nodes_buffer_size = octree_nodes_.size() * sizeof(uint32_t);
    io_nodes_buffer_[0] = std::make_unique<GlBuffer>(nodes_buffer_size);
    io_nodes_buffer_[1] = std::make_unique<GlBuffer>(nodes_buffer_size);
    visible_nodes_buffer_ = std::make_unique<GlBuffer>(nodes_buffer_size, GL_MAP_READ_BIT);
}

OctreeHiZRenderer::OctreeNode::OctreeNode(Bbox bbox) : bbox(bbox) {
    instances = {},
    std::fill_n(ch, 8, -1);
    level = 0;
}

void OctreeHiZRenderer::GenerateHiZ() {
    glCopyImageSubData(rasterizer_.GetDepthTarget()->Id(), GL_TEXTURE_2D, 0, 0, 0, 0,
        prev_depth_->Id(), GL_TEXTURE_2D, 0, 0, 0, 0, prev_depth_->Width(), prev_depth_->Height(), 1);

    glUseProgram(hiz_gen_program_->Id());

    uint32_t level = 0;
    uint32_t width = prev_depth_->Width();
    uint32_t height = prev_depth_->Height();
    while (width > 1 || height > 1) {
        ++level;
        width = width == 1 ? 1 : width / 2;
        height = height == 1 ? 1 : height / 2;

        glBindImageTexture(0, prev_depth_->Id(), level - 1, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);
        glBindImageTexture(1, prev_depth_->Id(), level, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32I);
        glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    glUseProgram(0);
}
