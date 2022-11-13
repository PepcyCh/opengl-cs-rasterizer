#include "renderer.hpp"

#include "basic.hpp"
#include "simple_hiz.hpp"
#include "octree_hiz.hpp"

std::unique_ptr<Renderer> Renderer::CreateRenderer(Rasterizer &rasterizer, const Scene &scene, RendererType type) {
    switch (type) {
        case RendererType::eBasic:
            return std::make_unique<BasicRenderer>(rasterizer, scene);
        case RendererType::eSimpleHiZ:
            return std::make_unique<SimpleHiZRenderer>(rasterizer, scene);
        case RendererType::eOctreeHiZ:
            return std::make_unique<OctreeHiZRenderer>(rasterizer, scene);
    }
    abort();
}
