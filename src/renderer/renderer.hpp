#pragma once

#include "scene/scene.hpp"
#include "rasterizer/rasterizer.hpp"

enum struct RendererType {
    eBasic,
    eSimpleHiZ,
    eOctreeHiZ,
};

inline constexpr const char *kRendererTypeName[3] = {
    "Basic",
    "Simple Hi-Z",
    "Octree Hi-Z",
};

class Renderer {
public:
    Renderer(Rasterizer &rasterizer, const Scene &scene) : rasterizer_(rasterizer), scene_(scene) {}
    virtual ~Renderer() = default;

    virtual void RenderScene() = 0;

    virtual void DrawUi() {}

    static std::unique_ptr<Renderer> CreateRenderer(Rasterizer &rasterizer, const Scene &scene,
        RendererType type = RendererType::eBasic);

protected:
    Rasterizer &rasterizer_;
    const Scene &scene_;
};
