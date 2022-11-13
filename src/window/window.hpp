#pragma once

#include <functional>

#include <glad/glad.h>
#include <glfw/glfw3.h>

class Window {
public:
    Window(uint32_t width, uint32_t height, const char *title);
    ~Window();

    void MainLoop(const std::function<void()> &func);

    using MouseCallback = std::function<void(uint32_t, float, float, float, float)>;
    enum Mouse {
        eMouseLeft = 1,
        eMouseRight = 2,
        eMouseMiddle = 4,
    };
    void SetMouseCallback(const MouseCallback &callback);

    using KeyCallback = std::function<void(int, int)>;
    void SetKeyCallback(const KeyCallback &callback);

    using ResizeCallback = std::function<void(uint32_t, uint32_t)>;
    void SetResizeCallback(const ResizeCallback &callback);

    void SetTitle(const char *title);

private:
    friend void GlfwCursorPosCallback(GLFWwindow *window, double x, double y);
    friend void GlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    friend void GlfwResizeCallback(GLFWwindow *window, int width, int height);

    uint32_t width_;
    uint32_t height_;

    GLFWwindow *window_ = nullptr;

    MouseCallback mouse_callback_ = [](uint32_t, float, float, float, float) {};
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;

    KeyCallback key_callback_ = [](int, int) {};

    ResizeCallback resize_callback_ = [](uint32_t, uint32_t) {};
};
