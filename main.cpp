#include <chrono>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "ParticleRenderer.h"
#include "SPHSystem.h"

int main() {

    int frameCount = 0;
    const int maxFrames = 1000;
    float totalDeltaTime = 0.0f; // Sum of all frame times

    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "SPH Fluid", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    glEnable(GL_PROGRAM_POINT_SIZE); // Important for seeing particles

    Shader shader; // uses internal shaders
    shader.use();

    // Camera setup
    glm::vec3 cameraPos = glm::vec3(2.0f, 1.0f, 0.5f); // Move camera to the +X axis
    glm::vec3 cameraTarget = glm::vec3(0.5f, 0.5f, 0.5f); // Look toward the center
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); // Keep up vector pointing along Y

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, up);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.01f, 100.0f);

    shader.setMat4("view", glm::value_ptr(view));
    shader.setMat4("projection", glm::value_ptr(proj));

    // Simulation
    SPHSystem system;
    ParticleRenderer renderer;

    std::vector<glm::vec3> positions;

    while (!glfwWindowShouldClose(window) && frameCount < maxFrames) {
        auto frameStart = std::chrono::high_resolution_clock::now(); // Frame start time
        
        system.computeDensityPressure();
        system.computeForces();
        system.integrate();
    
        positions.clear();
        for (const auto& p : system.particles)
            positions.push_back(p.position);
        renderer.update(positions);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        renderer.render();
    
        glfwSwapBuffers(window);
        glfwPollEvents();

        auto frameEnd = std::chrono::high_resolution_clock::now(); // Frame end time
        std::chrono::duration<float> deltaTime = frameEnd - frameStart;
    
        totalDeltaTime += deltaTime.count(); // Add this frame's time (in seconds)
        frameCount++;
    }

    float averageDeltaTime = totalDeltaTime / frameCount;
    std::cout << "Simulation complete.\n";
    std::cout << "Total time: " << totalDeltaTime << " seconds\n";
    std::cout << "Average time per frame: " << averageDeltaTime << " seconds\n";

    glfwTerminate();
    return 0;
}
