#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 position  = {0.f, 1.f, 3.f};
    glm::vec3 target    = {0.f, 0.f, 0.f};
    float     fovDeg    = 60.f;
    float     nearPlane = 0.1f;
    float     farPlane  = 100.f;

    glm::mat4 getView() const {
        return glm::lookAt(position, target, glm::vec3(0, 1, 0));
    }

    glm::mat4 getProjection(float aspect) const {
        auto proj = glm::perspective(glm::radians(fovDeg), aspect, nearPlane, farPlane);
        proj[1][1] *= -1;  // Vulkan Y-flip
        return proj;
    }
};
