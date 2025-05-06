#include "ContainerRenderer.h"
#include <glm/gtc/matrix_transform.hpp>

// Simple cube corners & line indices (wireframe)
static const float cubeVerts[] = {
    0,0,0,  1,0,0,  1,1,0,  0,1,0,
    0,0,1,  1,0,1,  1,1,1,  0,1,1
};
static const unsigned int cubeIdx[] = {
    0,1, 1,2, 2,3, 3,0,   // bottom
    4,5, 5,6, 6,7, 7,4,   // top
    0,4, 1,5, 2,6, 3,7    // vertical
};

ContainerRenderer::ContainerRenderer() {}
ContainerRenderer::~ContainerRenderer(){
    if (shader) delete shader;
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

void ContainerRenderer::init(const char* vertPath,
                             const char* fragPath)
{
    // 1) compile shaders
    shader = new Shader(vertPath, fragPath);

    // 2) gen & fill buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1,       &VBO);
    glGenBuffers(1,       &EBO);

    glBindVertexArray(VAO);
      glBindBuffer(GL_ARRAY_BUFFER, VBO);
      glBufferData(GL_ARRAY_BUFFER,
                   sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                   sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

      // position @ location 0
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT,
                            GL_FALSE, 3*sizeof(float),
                            (void*)0);
    glBindVertexArray(0);
}

void ContainerRenderer::render(const glm::mat4& view,
                               const glm::mat4& projection)
{
    shader->use();
    shader->setMat4("projection", glm::value_ptr(projection));
    shader->setMat4("view",       glm::value_ptr(view));

    // center on origin, scale to [–1,1]³
    glm::mat4 model = glm::scale(glm::mat4(1.0f),
                                 glm::vec3(2.0f));
    shader->setMat4("model", glm::value_ptr(model));

    shader->setVec3("color", color.x, color.y, color.z);

    glLineWidth(2.0f);
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}






