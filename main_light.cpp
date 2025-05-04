#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "ParticleRenderer.h"
#include "SPHSystem.h"

int main() {
    // Initialize GLFW and create window
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on macOS
    #endif


    GLFWwindow* window = glfwCreateWindow(800, 600, "SPH Fluid - Realistic Water", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();

    // Configure OpenGL state for transparency and depth testing
    glEnable(GL_PROGRAM_POINT_SIZE);               // Allow setting point size in vertex shader
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Disable depth writes for transparent blending of water particles

    // Create shader program (with built-in source for water simulation)
    Shader shader;
    shader.use();

    // Camera setup
    glm::vec3 cameraPos   = glm::vec3(2.0f, 1.0f, 0.5f);   // Camera position in world space
    glm::vec3 cameraTarget= glm::vec3(0.5f, 0.5f, 0.5f);   // Look at the center of the scene
    glm::vec3 up          = glm::vec3(0.0f, 1.0f, 0.0f);   // Up vector
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, up);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.01f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f); // Identity model matrix (no additional transform)

    // Pass transformation matrices to the shader
    shader.setMat4("projection", glm::value_ptr(projection));
    shader.setMat4("view", glm::value_ptr(view));
    shader.setMat4("model", glm::value_ptr(model));

    // Pass camera and lighting uniforms to shader
    shader.setVec3("lightPos", 0.0f, 0.0f, 0.0f);    // Light source position (e.g., a corner of the scene)
    shader.setVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z); // Camera position (for viewDir)
    shader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);  // Light color (white)
    shader.setVec3("waterColor", 0.2f, 0.6f, 1.0f);  // Base water color (slightly blue)

    // Pass screen parameters for proper point size calculation in shader
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    shader.setFloat("screenHeight", (float)screenHeight);
    shader.setFloat("fov", 45.0f); // vertical field-of-view in degrees

    // Load environment cubemap for reflections (not shown: image loading)
    // Assume we have a function loadCubemap that returns a cubemap texture ID
    // and a set of 6 image file paths for the cubemap faces.
    // GLuint cubemapTex = loadCubemap(faceImages);
    // For this example, we assume cubemapTex is already loaded with an environment.
    GLuint cubemapTex = 0;
    glGenTextures(1, &cubemapTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
    // (In a real application, load each cube face with image data here)
    // Set some default texture parameters for completeness
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // Bind cubemap to texture unit 0 and tell shader
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
    shader.setInt("skybox", 0);

    // Initialize simulation and renderer
    SPHSystem system;             // CPU-based SPH fluid system
    ParticleRenderer renderer;    // Renderer for fluid particles

    // Gather initial particle positions and initialize renderer buffer
    std::vector<glm::vec3> positions;
    std::vector<float> foamFactors;
    positions.reserve(system.particles.size());
    foamFactors.reserve(system.particles.size());
    for (const auto& p : system.particles) {
        positions.push_back(p.position);
        foamFactors.push_back(0.0f); // initial foam = 0 (no foam at start)
    }
    renderer.update(positions, foamFactors);

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Step the fluid simulation (CPU)
        system.computeDensityPressure();
        system.computeForces();
        system.integrate();

        // Prepare data for rendering
        positions.clear();
        foamFactors.clear();
        positions.reserve(system.particles.size());
        foamFactors.reserve(system.particles.size());
        // Calculate foam based on particle velocity (simple heuristic)
        float foamThreshold = 1.0f;      // velocity magnitude above which foam starts
        float maxFoamSpeed = 5.0f;       // velocity at which foam saturates
        glm::vec3 camDir = glm::normalize(cameraPos - cameraTarget); // camera direction (for sorting)
        for (const auto& p : system.particles) {
            positions.push_back(p.position);
            // Compute foam factor [0,1] based on speed (high speed -> more foam)
            float speed = glm::length(p.velocity);
            float foam = (speed - foamThreshold) / (maxFoamSpeed - foamThreshold);
            if (foam < 0.0f) foam = 0.0f;
            if (foam > 1.0f) foam = 1.0f;
            foamFactors.push_back(foam);
        }
        // Depth sort particles for proper transparency (far to near)
        // Create index list and sort by camera distance
        std::vector<size_t> indices(positions.size());
        for (size_t i = 0; i < indices.size(); ++i) indices[i] = i;
        glm::vec3 camPos = cameraPos; // copy to local for lambda capture
        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            float distA = glm::distance(camPos, positions[a]);
            float distB = glm::distance(camPos, positions[b]);
            return distA > distB; // far first
        });
        // Reorder positions and foamFactors according to sorted indices
        std::vector<glm::vec3> sortedPositions;
        std::vector<float> sortedFoam;
        sortedPositions.reserve(positions.size());
        sortedFoam.reserve(foamFactors.size());
        for (size_t idx : indices) {
            sortedPositions.push_back(positions[idx]);
            sortedFoam.push_back(foamFactors[idx]);
        }

        // Update GPU buffers with sorted particle data
        renderer.update(sortedPositions, sortedFoam);

        // Clear the frame (color and depth buffers)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black background (environment seen via cubemap)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render water particles
        shader.use();
        renderer.render();

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup and exit
    glfwTerminate();
    return 0;
}
