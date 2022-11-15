#pragma once

#include <functional>

#include "model.hpp"

class Scene {
public:
    struct Instance {
        size_t model;
        glm::mat4 transform;
        Bbox bbox;
    };

    Scene(const std::filesystem::path &scene_path);

    Bbox Bbox() const { return bbox_; }
    glm::vec3 Centroid() const { return bbox_.Centroid(); }
    float Extent() const { return bbox_.Extent(); }

    const Instance &GetInstance(size_t i) const { return instances_[i]; }
    const Model &GetModel(size_t i) const { return models_[i]; }

    size_t InstancesCount() const { return instances_.size(); }
    void ForEachInstance(const std::function<void(const Instance &, const Model &)> &func) const;

private:
    void CalcBbox();

    std::vector<Model> models_;
    std::vector<Instance> instances_;
    
    struct Bbox bbox_;
};
