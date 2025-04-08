#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "ParticleRenderer.h"
#include "SPHSystem.h"

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "SPH Fluid", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    glEnable(GL_PROGRAM_POINT_SIZE); // Important for seeing particles

    Shader shader; // uses internal shaders
    shader.use();

    // Camera setup
    // glm::vec3 cameraPos = glm::vec3(0.5f, 0.5f, 2.0f);
    // glm::vec3 cameraTarget = glm::vec3(0.5f, 0.5f, 0.5f);
    // glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 cameraPos = glm::vec3(2.0f, 0.5f, 0.5f); // Move camera to the +X axis
    glm::vec3 cameraTarget = glm::vec3(0.5f, 0.5f, 0.5f); // Look toward the center
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); // Keep up vector pointing along Y

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, up);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.01f, 100.0f);

    shader.setMat4("view", glm::value_ptr(view));
    shader.setMat4("projection", glm::value_ptr(projection));

    // Simulation
    SPHSystem system(5, 5, 5, 0.05f); // 1000 particles with 0.05 spacing
    ParticleRenderer renderer;

    // Extract initial positions
    std::vector<glm::vec3> positions;
    for (const auto& p : system.particles)
        positions.push_back(p.position);
    renderer.update(positions);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        system.computeDensityPressure();
        system.computeForces();
        system.integrate();

        // Update renderer with new positions
        positions.clear();
        for (const auto& p : system.particles)
            positions.push_back(p.position);
        renderer.update(positions);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        renderer.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
