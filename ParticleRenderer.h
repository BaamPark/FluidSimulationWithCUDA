#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "SPHParameters.h"

class ParticleRenderer {
public:
    ParticleRenderer();
    ~ParticleRenderer();

    void update(const std::vector<glm::vec3>& positions);
    void render();

private:
    GLuint vao, vbo;
    size_t maxParticles;
};
