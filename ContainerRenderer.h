#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"

class ContainerRenderer {
public:
    ContainerRenderer();
    ~ContainerRenderer();

    // Call once after GL context is ready
    void init(const char* vertPath = "./container.vert",
              const char* fragPath = "./container.frag");

    // Call every frame, using your current view/projection
    void render(const glm::mat4& view,
                const glm::mat4& projection);

    void setColor(const glm::vec3& col) { color = col; }

                

private:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    Shader* shader = nullptr;
    Shader containerShader;
    glm::vec3 color = glm::vec3(1.0f);
};


