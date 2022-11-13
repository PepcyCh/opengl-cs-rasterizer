#include "basic.hpp"

void BasicRenderer::RenderScene() {
    scene_.ForEachInstance([this](const Scene::Instance &inst, const Model &model) {
        rasterizer_.SetMatrixModel(inst.transform);
        rasterizer_.SetPositionBuffer(model.PositionBuffer());
        rasterizer_.SetNormalBuffer(model.NormalBuffer());
        rasterizer_.SetIndexBuffer(model.IndexBuffer());
        rasterizer_.DrawIndexed(model.IndicesCount());
    });
}
