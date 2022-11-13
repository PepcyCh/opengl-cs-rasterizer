#pragma once

#include <filesystem>

class GlShader {
public:
    GlShader(const char *source, uint32_t type);
    GlShader(const uint8_t *binary, uint32_t length, uint32_t type);
    ~GlShader();

private:
    friend class GlProgram;

    uint32_t shader_;
};

class GlProgram {
public:
    GlProgram();
    ~GlProgram();

    void Attach(const GlShader &shader);
    void Link();

    uint32_t Id() const { return program_; }

private:
    uint32_t program_;
};
