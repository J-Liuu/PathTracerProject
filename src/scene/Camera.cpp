#include "Camera.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

namespace pt {

vec3 Camera::forward() const {
    const float y = glm::radians(yaw);
    const float p = glm::radians(pitch);
    return glm::normalize(vec3(std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)));
}

vec3 Camera::right() const { return glm::normalize(glm::cross(forward(), vec3(0.0f, 1.0f, 0.0f))); }

void Camera::move(const vec3& localDir, float distance) {
    position += (right() * localDir.x + vec3(0.0f, 1.0f, 0.0f) * localDir.y + forward() * localDir.z) * distance;
}

void Camera::rotate(float dYawDeg, float dPitchDeg) {
    yaw += dYawDeg;
    pitch = std::clamp(pitch + dPitchDeg, -89.0f, 89.0f);
}

mat4 Camera::view() const { return glm::lookAt(position, position + forward(), vec3(0.0f, 1.0f, 0.0f)); }

mat4 Camera::projection(float aspect) const {
    mat4 p = glm::perspective(glm::radians(verticalFovDeg), aspect, nearPlane, farPlane);
    p[1][1] *= -1.0f;  // Vulkan's Y points down
    return p;
}

}