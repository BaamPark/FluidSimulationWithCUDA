#pragma once
#include <glm/glm.hpp>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 force;
    float density;
    float pressure;

    Particle(const glm::vec3& pos)
        : position(pos), velocity(0.0f), force(0.0f), density(0.0f), pressure(0.0f) {}
};
