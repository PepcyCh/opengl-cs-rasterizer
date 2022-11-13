#include "scene.hpp"

#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rasterizer/rasterizer.hpp"

Scene::Scene(const std::filesystem::path &scene_path) {
    auto ext = scene_path.extension().string();
    if (ext == ".obj") {
        models_.emplace_back(scene_path);
        instances_.push_back(Instance {
            .model = 0,
            .transform = glm::mat4(1.0f),
        });
    } else if (ext == ".json") {
        auto scene_json = nlohmann::json::parse(std::ifstream(scene_path));
        auto models_json = scene_json["models"];
        std::unordered_map<std::string, size_t> model_map;
        for (auto &model_json : models_json) {
            model_map[model_json["name"]] = models_.size();
            auto model_file_path = scene_path;
            model_file_path.replace_filename(model_json["file"]);
            models_.emplace_back(model_file_path);
        }
        auto instances_json = scene_json["instances"];
        for (auto &instance_json : instances_json) {
            glm::mat4 trans = glm::mat4(1.0f);
            if (instance_json.contains("translate")) {
                auto translate = instance_json["translate"];
                trans = glm::translate(trans,
                    glm::vec3(translate[0].get<float>(), translate[1].get<float>(), translate[2].get<float>()));
            }
            if (instance_json.contains("rotate")) {
                auto rotate = instance_json["rotate"];
                trans = glm::rotate(trans, glm::radians(rotate[2].get<float>()), glm::vec3(0.0f, 0.0f, 1.0f));
                trans = glm::rotate(trans, glm::radians(rotate[1].get<float>()), glm::vec3(0.0f, 1.0f, 0.0f));
                trans = glm::rotate(trans, glm::radians(rotate[0].get<float>()), glm::vec3(1.0f, 0.0f, 0.0f));
            }
            if (instance_json.contains("scale")) {
                auto scale = instance_json["scale"];
                trans = glm::scale(trans,
                    glm::vec3(scale[0].get<float>(), scale[1].get<float>(), scale[2].get<float>()));
            }
            instances_.push_back(Instance {
                .model = model_map[instance_json["model"]],
                .transform = trans,
            });
        }
    }

    CalcBbox();
}

void Scene::ForEachInstance(const std::function<void(const Instance &, const Model &)> &func) const {
    for (const auto &inst : instances_) {
        const auto &model = models_[inst.model];
        func(inst, model);
    }
}

void Scene::CalcBbox() {
    bbox_.Empty();
    for (auto &inst : instances_) {
        inst.bbox = models_[inst.model].Bbox().TransformBy(inst.transform);
        bbox_.Merge(inst.bbox);
    }
}
