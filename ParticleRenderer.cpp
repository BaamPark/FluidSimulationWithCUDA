#include "ParticleRenderer.h"

ParticleRenderer::ParticleRenderer() {
    maxParticles = numX * numY * numZ;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(0);
}

ParticleRenderer::~ParticleRenderer() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void ParticleRenderer::update(const std::vector<glm::vec3>& positions) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3), positions.data());
}

void ParticleRenderer::render() {
    glBindVertexArray(vao);
    //GL_POINTS draws 2D square
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(maxParticles));
    glBindVertexArray(0);
}
