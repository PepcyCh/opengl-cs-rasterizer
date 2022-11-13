#pragma once

#include <memory>

#include "glh/program.hpp"
#include "defines.hpp"

inline const std::filesystem::path kShaderSourceDir = std::filesystem::path(kProjectSourceDir) / "shaders";

void CreateComputeProgram(std::unique_ptr<GlProgram> &program, const std::filesystem::path &path);

void CreateComputeProgramBin(std::unique_ptr<GlProgram> &program, const std::filesystem::path &path);
