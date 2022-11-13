#include <string>
#include <iostream>
#include <fstream>

#include <imgui.h>

#include "defines.hpp"
#include "renderer/renderer.hpp"
#include "camera/camera.hpp"
#include "window/window.hpp"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " <path-to-scene-file>" << std::endl;
        return -1;
    }

    std::filesystem::path obj_path(argv[1]);
    if (!std::filesystem::exists(obj_path)) {
        std::cout << "scene file '" << argv[1] << "' doesn't exist" << std::endl;
        return -1;
    }

    uint32_t window_width = 1600;
    uint32_t window_height = 900;
    Window window(window_width, window_height, "a1-dpeth");
    glViewport(0, 0, window_width, window_height);

    Scene scene(obj_path);

    OrbitCamera camera(scene.Centroid(), scene.Extent() * 1.2f, static_cast<float>(window_width) / window_height);

    Rasterizer rasterizer(window_width, window_height);
    rasterizer.SetClearColor(0.2f, 0.3f, 0.5f, 1.0f);

    auto curr_renderer_type = RendererType::eBasic;
    std::unique_ptr<Renderer> renderers[] = {
        Renderer::CreateRenderer(rasterizer, scene, RendererType::eBasic),
        Renderer::CreateRenderer(rasterizer, scene, RendererType::eSimpleHiZ),
        Renderer::CreateRenderer(rasterizer, scene, RendererType::eOctreeHiZ),
    };
    auto renderer = renderers[static_cast<size_t>(curr_renderer_type)].get();

    auto color_buffer = std::make_unique<GlTexture2D>(GL_RGBA8, window_width, window_height, 1);
    auto depth_buffer = std::make_unique<GlTexture2D>(GL_R32F, window_width, window_height, 1);
    rasterizer.SetViewport(window_width, window_height);
    rasterizer.SetColorTarget(color_buffer.get());
    rasterizer.SetDepthTarget(depth_buffer.get());

    window.SetResizeCallback([&](uint32_t width, uint32_t height) {
        window_width = width;
        window_height = height;
        glViewport(0, 0, window_width, window_height);

        camera.SetAspect(static_cast<float>(window_width) / window_height);

        color_buffer = std::make_unique<GlTexture2D>(GL_RGBA8, window_width, window_height, 1);
        depth_buffer = std::make_unique<GlTexture2D>(GL_R32F, window_width, window_height, 1);
        rasterizer.SetViewport(window_width, window_height);
        rasterizer.SetColorTarget(color_buffer.get());
        rasterizer.SetDepthTarget(depth_buffer.get());
    });

    window.SetMouseCallback([&](uint32_t state, float x, float y, float last_x, float last_y) {
        if ((state & Window::eMouseMiddle) != 0) {
            float dx = glm::radians(0.25f * (x - last_x));
            float dy = glm::radians(0.25f * (y - last_y));
            camera.Rotate(dx, dy);
        }
        if ((state & Window::eMouseRight) != 0) {
            float dx = 0.005f * (x - last_x);
            float dy = 0.005f * (y - last_y);
            camera.Forward(dx - dy);
        }
    });

    auto display_program = std::make_unique<GlProgram>();
    {
        const char *display_vert = R"(
            #version 460

            layout(location = 0) out vec2 texcoord;

            void main() {
                texcoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
                gl_Position = vec4(texcoord.x * 2.0 - 1.0, 1.0 - texcoord.y * 2.0, 0.0, 1.0);
            }
        )";
        GlShader vert(display_vert, GL_VERTEX_SHADER);

        const char *display_frag = R"(
            #version 460

            layout(location = 0) in vec2 texcoord;

            layout(location = 0) out vec4 frag_color;

            layout(binding = 0) uniform sampler2D color_image;

            void main() {
                frag_color = texture(color_image, texcoord);
            }
        )";
        GlShader frag(display_frag, GL_FRAGMENT_SHADER);

        display_program->Attach(vert);
        display_program->Attach(frag);
        display_program->Link();
    }

    uint32_t empty_vao;
    glCreateVertexArrays(1, &empty_vao);

    window.MainLoop([&]() {
        rasterizer.ClearBuffers();

        rasterizer.SetMatrixProj(camera.Proj());
        rasterizer.SetMatrixView(camera.View());

        renderer->RenderScene();

        glUseProgram(display_program->Id());
        glBindVertexArray(empty_vao);
        glBindTextureUnit(0, color_buffer->Id());
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glUseProgram(0);

        if (ImGui::Begin("Status")) {
            float fps = ImGui::GetIO().Framerate;
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / fps, fps);

            ImGui::Separator();

            auto temp_renderer = static_cast<int>(curr_renderer_type);
            ImGui::Combo("Renderer", &temp_renderer, kRendererTypeName, 3);
            curr_renderer_type = static_cast<RendererType>(temp_renderer);
            renderer = renderers[temp_renderer].get();

            renderer->DrawUi();
        }
        ImGui::End();
    });

    glDeleteVertexArrays(1, &empty_vao);
    
    return 0;
}
