#include "resource.hpp"

#include <glad/glad.h>

GlBuffer::GlBuffer(uint64_t size, uint32_t usage, const void *data) : size_(size) {
    glCreateBuffers(1, &gl_buffer_);
    glNamedBufferStorage(gl_buffer_, size, data, usage);
}

GlBuffer::~GlBuffer() {
    Unmap();
    glDeleteBuffers(1, &gl_buffer_);
}

void *GlBuffer::Map(bool write) {
    if (!mapped_ptr_) {
        mapped_ptr_ = glMapNamedBuffer(gl_buffer_, write ? GL_READ_WRITE : GL_READ_ONLY);
    }
    return mapped_ptr_;
}

void GlBuffer::Unmap() {
    if (mapped_ptr_) {
        glUnmapNamedBuffer(gl_buffer_);
        mapped_ptr_ = nullptr;
    }
}

GlTexture2D::GlTexture2D(uint32_t format, uint32_t width, uint32_t height, uint32_t levels)
    : width_(width), height_(height), levels_(levels), format_(format) {
    if (levels == 0) {
        uint32_t temp_width = width;
        uint32_t temp_height = height;
        levels_ = 1;
        while (temp_width > 1 || temp_height > 1) {
            ++levels_;
            temp_width = temp_width == 1 ? 1 : temp_width / 2;
            temp_height = temp_height == 1 ? 1 : temp_height / 2;
        }
    }
    glCreateTextures(GL_TEXTURE_2D, 1, &gl_texture_);
    glTextureStorage2D(gl_texture_, levels_, format, width, height);
}

GlTexture2D::~GlTexture2D() {
    glDeleteTextures(1, &gl_texture_);
}
