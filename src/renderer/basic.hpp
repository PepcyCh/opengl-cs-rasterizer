#pragma once

#include "renderer.hpp"

class BasicRenderer final : public Renderer {
public:
    BasicRenderer(Rasterizer &rasterizer, const Scene &scene) : Renderer(rasterizer, scene) {}

    void RenderScene() override;
};
