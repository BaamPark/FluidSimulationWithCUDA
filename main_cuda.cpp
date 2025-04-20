// main_cuda.cpp  – GPU‑only entry point
// -------------------------------------------------------------
// Identical to your original main.cpp except that we always use
// SPHSystemCUDA and drop the CPU/GPU #ifdef switch.
// -------------------------------------------------------------

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "ParticleRenderer.h"

#include "SPHSystemCUDA.cuh"          // <- CUDA implementation
using SPHSim = SPHSystemCUDA;        //   alias for brevity

int main() {
    // ---------------------------------------------------------
    // GLFW / OpenGL setup
    // ---------------------------------------------------------
    glfwInit();
    GLFWwindow* window =
        glfwCreateWindow(800, 600, "SPH Fluid (CUDA)", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    glEnable(GL_PROGRAM_POINT_SIZE);   // show GL_POINTS as squares

    // ---------------------------------------------------------
    // Shader and camera
    // ---------------------------------------------------------
    Shader shader;
    shader.use();

    glm::vec3 cameraPos    = glm::vec3(2.0f, 0.5f, 0.5f);
    glm::vec3 cameraTarget = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 up           = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, up);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                      800.0f / 600.0f,
                                      0.01f, 100.0f);

    shader.setMat4("view",       glm::value_ptr(view));
    shader.setMat4("projection", glm::value_ptr(proj));

    // ---------------------------------------------------------
    // Simulation and renderer
    // ---------------------------------------------------------
    SPHSim system(10, 10, 10, 0.05f);   // 1 000 particles on the GPU
    ParticleRenderer renderer;

    std::vector<glm::vec3> positions;
    for (const auto& p : system.particles) positions.push_back(p.position);
    renderer.update(positions);

    // ---------------------------------------------------------
    // Main loop
    // ---------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {
        // physics (GPU)
        system.computeDensityPressure();
        system.computeForces();
        system.integrate();

        // copy positions vector (already filled inside integrate)
        positions.clear();
        for (const auto& p : system.particles) positions.push_back(p.position);
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
