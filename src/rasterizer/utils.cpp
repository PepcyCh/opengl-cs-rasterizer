#include "utils.hpp"

#include <fstream>

#include <glad/glad.h>

namespace {

std::string ReadAll(const std::filesystem::path &path) {
    std::ifstream fin(path);
    fin.seekg(0, std::ios::end);
    size_t length = fin.tellg();
    fin.seekg(0, std::ios::beg);
    std::string content(length, '\0');
    fin.read(content.data(), length);
    return content;
}

std::vector<uint8_t> ReadAllBin(const std::filesystem::path &path) {
    std::ifstream fin(path);
    fin.seekg(0, std::ios::end);
    size_t length = fin.tellg();
    fin.seekg(0, std::ios::beg);
    std::vector<uint8_t> content(length);
    fin.read(reinterpret_cast<char *>(content.data()), length);
    return content;
}

}

void CreateComputeProgram(std::unique_ptr<GlProgram> &program, const std::filesystem::path &path) {
    auto source = ReadAll(path);
    GlShader shader_module(source.c_str(), GL_COMPUTE_SHADER);
    
    program = std::make_unique<GlProgram>();
    program->Attach(shader_module);
    program->Link();
}

void CreateComputeProgramBin(std::unique_ptr<GlProgram> &program, const std::filesystem::path &path) {
    auto source = ReadAllBin(path);
    GlShader shader_module(source.data(), source.size(), GL_COMPUTE_SHADER);
    program = std::make_unique<GlProgram>();
    program->Attach(shader_module);
    program->Link();
}
