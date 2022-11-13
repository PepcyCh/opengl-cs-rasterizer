#pragma once

#include <cstdint>

class GlBuffer {
public:
    GlBuffer(uint64_t size, uint32_t usage = 0, const void *data = nullptr);
    ~GlBuffer();

    uint32_t Id() const { return gl_buffer_; }

    uint64_t Size() const { return size_; }

    void *Map(bool write = false);
    template <typename T>
    T *TypedMap(bool write = false) {
        return reinterpret_cast<T *>(Map(write));
    }
    void Unmap();

private:
    uint32_t gl_buffer_;
    uint64_t size_;
    void *mapped_ptr_ = nullptr;
};

class GlTexture2D {
public:
    GlTexture2D(uint32_t format, uint32_t width, uint32_t height, uint32_t levels = 0);
    ~GlTexture2D();

    uint32_t Id() const { return gl_texture_; }

    uint32_t Width() const { return width_; }
    uint32_t Height() const { return height_; }
    uint32_t Levels() const { return levels_; }
    uint32_t Format() const { return format_; }

private:
    uint32_t gl_texture_;
    uint32_t width_;
    uint32_t height_;
    uint32_t levels_;
    uint32_t format_;
};
