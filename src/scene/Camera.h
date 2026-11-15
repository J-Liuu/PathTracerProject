#pragma once

#include "../core/Types.h"

namespace pt {
// angles are in degrees
class Camera {
public:
    vec3  position{ 0.0f, 1.0f, 3.5f };
    float yaw = -90.0f;
    float pitch = 0.0f;
    float verticalFovDeg = 45.0f;
    float nearPlane = 0.01f;
    float farPlane = 1000.0f;

    vec3 forward() const;
    vec3 right() const;
    vec3 up() const { return glm::cross(right(), forward()); }

    void move(const vec3& localDir, float distance);   // localDir: x=right, y=up, z=forward
    void rotate(float dYawDeg, float dPitchDeg);

    mat4 view() const;
    mat4 projection(float aspect) const;   // Vulkan clip space, Y flipped
};

}