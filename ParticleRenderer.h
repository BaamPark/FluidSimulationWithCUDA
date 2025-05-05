#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "SPHParameters.h"

// Renderer for SPH particles with water rendering enhancements
class ParticleRenderer {
public:
    ParticleRenderer();
    ~ParticleRenderer();

    // Update particle positions and per-particle foam factors (called each frame)
    void update(const std::vector<glm::vec3>& positions, const std::vector<float>& foamFactors);

    // Render particles as water (uses OpenGL point sprites and shader for water effects)
    void render();

private:
    GLuint vao;
    GLuint posVBO;
    GLuint foamVBO;
    size_t maxParticles;
    size_t count; // number of particles to draw (updated each frame)
};