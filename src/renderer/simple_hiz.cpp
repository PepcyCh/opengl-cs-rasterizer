#include "simple_hiz.hpp"

#include <glad/glad.h>
#include <imgui.h>

#include "rasterizer/utils.hpp"

namespace {

struct CullResults {
    uint32_t num_total;
    uint32_t num_visible;
    uint32_t num_culled;
};

struct alignas(16) CullParams {
    glm::mat4 view;
    glm::mat4 proj;
    uint32_t index_offset;
};

constexpr uint32_t kComputeWorkGroupSize = 256;

}

SimpleHiZRenderer::SimpleHiZRenderer(Rasterizer &rasterizer, const Scene &scene) : Renderer(rasterizer, scene) {
    CreateComputeProgram(fill_id_map_program_, kShaderSourceDir / "simple_hiz/fill_inst_id_map.comp");
    CreateComputeProgram(hiz_gen_program_, kShaderSourceDir / "hiz_gen.comp");
    CreateComputeProgram(hiz_cull_program_, kShaderSourceDir / "simple_hiz/cull.comp");

    bbox_buffer_ = std::make_unique<GlBuffer>(scene.InstancesCount() * sizeof(float) * 6,
        GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    auto p_bbox = bbox_buffer_->TypedMap<float>(true);
    scene.ForEachInstance([&p_bbox](const Scene::Instance &inst, const Model &model) {
        *p_bbox++ = inst.bbox.pmin.x;
        *p_bbox++ = inst.bbox.pmin.y;
        *p_bbox++ = inst.bbox.pmin.z;
        *p_bbox++ = inst.bbox.pmax.x;
        *p_bbox++ = inst.bbox.pmax.y;
        *p_bbox++ = inst.bbox.pmax.z;
    });
    bbox_buffer_->Unmap();

    output_instances_.resize(scene.InstancesCount());
    instances_id_map_buffer_ = std::make_unique<GlBuffer>(scene.InstancesCount() * sizeof(uint32_t), GL_MAP_READ_BIT);
    output_instance_buffer_ = std::make_unique<GlBuffer>(scene.InstancesCount() * sizeof(uint32_t), GL_MAP_READ_BIT);
    cull_result_buffer_ = std::make_unique<GlBuffer>(3 * sizeof(uint32_t), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

    camera_info_buffer_ = std::make_unique<GlBuffer>(sizeof(CullParams), GL_DYNAMIC_STORAGE_BIT);
}

void SimpleHiZRenderer::RenderScene() {
    auto depth_buffer = rasterizer_.GetDepthTarget();
    bool can_do_cull = true;
    if (!prev_depth_ || prev_depth_->Width() != depth_buffer->Width()
        || prev_depth_->Height() != depth_buffer->Height()) {
        prev_depth_ = std::make_unique<GlTexture2D>(depth_buffer->Format(), depth_buffer->Width(),
            depth_buffer->Height(), 0);
        can_do_cull = false;
    }

    CullResults cull_res {
        .num_total = static_cast<uint32_t>(scene_.InstancesCount()),
        .num_visible = 0,
        .num_culled = 0,
    };
    if (can_do_cull) {
        auto p_cull_res = cull_result_buffer_->TypedMap<CullResults>(true);
        *p_cull_res = cull_res;
        cull_result_buffer_->Unmap();

        glUseProgram(fill_id_map_program_->Id());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, instances_id_map_buffer_->Id());
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, cull_result_buffer_->Id(), 0, sizeof(uint32_t));
        glDispatchCompute((scene_.InstancesCount() + kComputeWorkGroupSize - 1) / kComputeWorkGroupSize, 1, 1);
        glUseProgram(0);

        CullParams camera {
            .view = rasterizer_.GetMatrixView(),
            .proj = rasterizer_.GetMatrixProj(),
            .index_offset = 0,
        };
        glNamedBufferSubData(camera_info_buffer_->Id(), 0, sizeof(CullParams), &camera);

        glUseProgram(hiz_cull_program_->Id());

        glBindTextureUnit(0, prev_depth_->Id());

        uint32_t storage_buffers[] = {
            bbox_buffer_->Id(),
            instances_id_map_buffer_->Id(),
            output_instance_buffer_->Id(),
            cull_result_buffer_->Id(),
        };
        glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 1, 4, storage_buffers);
        glBindBufferBase(GL_UNIFORM_BUFFER, 5, camera_info_buffer_->Id());

        glDispatchCompute((scene_.InstancesCount() + kComputeWorkGroupSize - 1) / kComputeWorkGroupSize, 1, 1);

        glUseProgram(0);

        cull_res = *cull_result_buffer_->TypedMap<CullResults>();
        cull_result_buffer_->Unmap();

        auto output = output_instance_buffer_->TypedMap<uint32_t>();
        std::copy_n(output, scene_.InstancesCount(), output_instances_.data());
        output_instance_buffer_->Unmap();
    } else {
        cull_res.num_visible = cull_res.num_total;
        for (uint32_t i = 0; i < scene_.InstancesCount(); i++) {
            output_instances_[i] = i;
        }
    }

    for (uint32_t i = 0; i < cull_res.num_visible; i++) {
        const auto &inst = scene_.GetInstance(output_instances_[i]);
        const auto &model = scene_.GetModel(inst.model);

        rasterizer_.SetMatrixModel(inst.transform);
        rasterizer_.SetPositionBuffer(model.PositionBuffer());
        rasterizer_.SetNormalBuffer(model.NormalBuffer());
        rasterizer_.SetIndexBuffer(model.IndexBuffer());
        rasterizer_.DrawIndexed(model.IndicesCount());
    }
    num_drawn_instances_ = cull_res.num_visible;
    
    GenerateHiZ();

    if (cull_res.num_culled > 0) {
        auto offset = cull_res.num_visible;
        cull_res.num_total = cull_res.num_culled;
        cull_res.num_culled = 0;
        cull_res.num_visible = 0;
        auto p_cull_res = cull_result_buffer_->TypedMap<CullResults>(true);
        *p_cull_res = cull_res;
        cull_result_buffer_->Unmap();

        CullParams camera {
            .view = rasterizer_.GetMatrixView(),
            .proj = rasterizer_.GetMatrixProj(),
            .index_offset = offset,
        };
        glNamedBufferSubData(camera_info_buffer_->Id(), 0, sizeof(CullParams), &camera);

        glUseProgram(hiz_cull_program_->Id());

        glBindTextureUnit(0, prev_depth_->Id());

        uint32_t storage_buffers[] = {
            bbox_buffer_->Id(),
            output_instance_buffer_->Id(),
            instances_id_map_buffer_->Id(),
            cull_result_buffer_->Id(),
        };
        glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 1, 4, storage_buffers);
        glBindBufferBase(GL_UNIFORM_BUFFER, 5, camera_info_buffer_->Id());

        glDispatchCompute((cull_res.num_total + kComputeWorkGroupSize - 1) / kComputeWorkGroupSize, 1, 1);

        glUseProgram(0);

        cull_res = *cull_result_buffer_->TypedMap<CullResults>();
        cull_result_buffer_->Unmap();

        auto output = instances_id_map_buffer_->TypedMap<uint32_t>();
        std::copy_n(output, cull_res.num_visible, output_instances_.data());
        instances_id_map_buffer_->Unmap();

        for (uint32_t i = 0; i < cull_res.num_visible; i++) {
            const auto &inst = scene_.GetInstance(output_instances_[i]);
            const auto &model = scene_.GetModel(inst.model);

            rasterizer_.SetMatrixModel(inst.transform);
            rasterizer_.SetPositionBuffer(model.PositionBuffer());
            rasterizer_.SetNormalBuffer(model.NormalBuffer());
            rasterizer_.SetIndexBuffer(model.IndexBuffer());
            rasterizer_.DrawIndexed(model.IndicesCount());
        }

        GenerateHiZ();

        num_drawn_instances_ += cull_res.num_visible;
    }
}

void SimpleHiZRenderer::DrawUi() {
    ImGui::Text("Culling: %d / %d", num_drawn_instances_, static_cast<uint32_t>(scene_.InstancesCount()));
}

void SimpleHiZRenderer::GenerateHiZ() {
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
