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
    GLuint normalVBO;
    size_t maxParticles;
};


// Why I added count variable
// By tracking count = positions.size() in update() and then doing glDrawArrays(GL_POINTS, 0,count), you guarantee that:
// The GPU only ever tries to draw the number of vertices you actually provided.