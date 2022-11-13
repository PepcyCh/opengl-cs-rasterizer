#include "camera.hpp"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr float kPi = 3.141592653589793238463f;

}

OrbitCamera::OrbitCamera(const glm::vec3 &look_at, float radius, float aspect) : look_at_(look_at), radius_(radius) {
    theta_ = 0.0f;
    phi_ = kPi * 0.5f;
    proj_ = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 1000.0f);
    proj_inv_ = glm::inverse(proj_);
    Update();
}

void OrbitCamera::Rotate(float delta_x, float delta_y) {
    theta_ -= delta_x;
    if (theta_ < 0.0) {
        theta_ += 2.0f * kPi;
    } else if (theta_ >= 2.0f * kPi) {
        theta_ -= 2.0f * kPi;
    }
    phi_ = std::clamp(phi_ + delta_y, 0.1f, kPi - 0.1f);
    Update();
}

void OrbitCamera::Forward(float delta) {
    radius_ = std::max(radius_ + delta, 0.1f);
    Update();
}

void OrbitCamera::SetAspect(float aspect) {
    proj_ = glm::perspective(glm::radians(45.0f), aspect, 0.001f, 100000.0f);
    proj_inv_ = glm::inverse(proj_);
    Update();
}

void OrbitCamera::Update() {
    pos_.x = radius_ * std::sin(phi_) * std::cos(theta_);
    pos_.y = radius_ * std::cos(phi_);
    pos_.z = radius_ * std::sin(phi_) * std::sin(theta_);
    view_ = glm::lookAt(pos_, look_at_, glm::vec3(0.0f, 1.0f, 0.0f));
    view_inv_ = glm::inverse(view_);
}
