#include "ParticleRenderer.h"
#include <GL/glew.h>

ParticleRenderer::ParticleRenderer() {
    // Determine maximum particles (from simulation parameters)
    maxParticles = numX * numY * numZ;

    // Generate Vertex Array Object and Vertex Buffer Objects for positions and foam
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &posVBO);
    glGenBuffers(1, &foamVBO);

    glBindVertexArray(vao);
    // Position VBO setup
    glBindBuffer(GL_ARRAY_BUFFER, posVBO);
    // Allocate buffer for max number of particle positions (dynamic since positions change)
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    // Attribute 0 = 3D position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Foam factor VBO setup
    glBindBuffer(GL_ARRAY_BUFFER, foamVBO);
    // Allocate buffer for max number of foam values (one float per particle)
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    // Attribute 1 = foam factor (as a single float)
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    // Unbind VAO
    glBindVertexArray(0);

    count = 0;
}

ParticleRenderer::~ParticleRenderer() {
    // Cleanup GPU resources
    glDeleteBuffers(1, &posVBO);
    glDeleteBuffers(1, &foamVBO);
    glDeleteVertexArrays(1, &vao);
}

void ParticleRenderer::update(const std::vector<glm::vec3>& positions, const std::vector<float>& foamFactors) {
    count = positions.size();
    // Update positions VBO with new particle positions
    glBindBuffer(GL_ARRAY_BUFFER, posVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(glm::vec3), positions.data());
    // Update foam VBO with new per-particle foam values
    glBindBuffer(GL_ARRAY_BUFFER, foamVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(float), foamFactors.data());
}

void ParticleRenderer::render() {
    // Draw all particles as points (each point expanded into a sphere in the shader)
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(count));
    glBindVertexArray(0);
}
