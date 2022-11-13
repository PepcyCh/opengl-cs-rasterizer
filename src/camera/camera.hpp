#pragma once

#include <glm/glm.hpp>

class OrbitCamera {
public:
    OrbitCamera(const glm::vec3 &look_at, float radius, float aspect);

    glm::vec3 Position() const { return pos_; }
    glm::mat4 Proj() const { return proj_; }
    glm::mat4 View() const { return view_; }

    void Rotate(float delta_x, float delta_y);
    void Forward(float delta);

    void SetAspect(float aspect);

private:
    void Update();

    glm::vec3 pos_;
    glm::vec3 look_at_;
    float theta_;
    float phi_;
    float radius_;
    glm::mat4 proj_;
    glm::mat4 proj_inv_;
    glm::mat4 view_;
    glm::mat4 view_inv_;
};
