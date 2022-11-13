# OpenGL Compute Shader Rasterizer

A simple compute shader fixed-function rasterizer with hierarchical z-buffer culling in OpenGL 4.6.

Usage:

```
cs-rasterizer <path-to-scene-file>
```

A scene file can be a `.obj` file or a `.json` scene file, see `scenes` folder for details.

## Build

CMake is used to build this project.

C++20 is needed.

## Used Thirdparty

* [glad](https://github.com/Dav1dde/glad)
* [glfw](https://github.com/glfw/glfw)
* [glm](https://github.com/g-truc/glm)
* [Dear ImGui](https://github.com/ocornut/imgui)
* [nlohmann json](https://github.com/nlohmann/json)
* [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)

## Details

For simplicity, shade fragments with world space normal.

3 rasterizer shaders are implemented:

1. Just loop in the bounding box of triangle. (`basic_z.comp`)
2. Scanline from top to down while maintaining horizontal boundary. (`scanline.comp`)
3. (Default) Push each triangle and corresponding horizontal boundary to per-line lists and dispatch another pass to draw. (`line_tile_pre.comp` and `line_tile_draw.comp`)

2 kinds of Hi-Z culling are implemented:
1. Check each instance's bounding box. (`simple_hiz`)
2. Do culling through scene octree. (`octree_hiz`)
