#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cmath>

// ----- SHADER -----
const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 projection;
uniform mat4 view;
void main() {
    gl_Position = projection * view * vec4(aPos, 1.0);
    gl_PointSize = 5.0;
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.2, 0.6, 1.0, 1.0);
}
)";

GLuint createShaderProgram() {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragment);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

// ----- PARTICLE SIM -----
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 force = glm::vec3(0.0f);
    float density = 0.0f;
    float pressure = 0.0f;
    Particle(const glm::vec3& pos) : position(pos) {}
};

const float REST_DENSITY = 1000.0f;
const float GAS_CONSTANT = 2000.0f;
const float VISCOSITY = 0.1f;
const float TIME_STEP = 0.005f;
const float MASS = 0.02f;
const float H = 0.045f;
const float GRAVITY = -9.81f;

float poly6(float r2) {
    float h2 = H * H;
    if (r2 >= h2) return 0.0f;
    float coeff = 315.0f / (64.0f * M_PI * pow(H, 9));
    float diff = h2 - r2;
    return static_cast<float>(coeff * diff * diff * diff);
}


glm::vec3 spikyGrad(const glm::vec3& rij) {
    float r = glm::length(rij);
    if (r == 0.0f || r > H) return glm::vec3(0.0f);

    float coeff = -45.0f / (M_PI * pow(H, 6));
    float factor = static_cast<float>(coeff * pow(H - r, 2));
    return factor * (rij / r);
}


float viscLaplacian(float r) {
    if (r >= H) return 0.0f;
    float coeff = 45.0f / (M_PI * pow(H, 6));
    return coeff * (H - r);
}

// Simulation container
std::vector<Particle> initParticles(int nx, int ny, int nz, float spacing) {
    std::vector<Particle> ps;
    for (int x = 0; x < nx; ++x)
        for (int y = 0; y < ny; ++y)
            for (int z = 0; z < nz; ++z)
                ps.emplace_back(glm::vec3(x * spacing, y * spacing, z * spacing));
    return ps;
}

void computeDensityPressure(std::vector<Particle>& ps) {
    for (auto& pi : ps) {
        pi.density = 0.0f;
        for (const auto& pj : ps) {
            glm::vec3 rij = pj.position - pi.position;
            float r2 = glm::dot(rij, rij);
            pi.density += MASS * poly6(r2);
        }
        pi.pressure = GAS_CONSTANT * (pi.density - REST_DENSITY);
    }
}

void computeForces(std::vector<Particle>& ps) {
    for (auto& pi : ps) {
        glm::vec3 fPressure(0.0f), fViscosity(0.0f);
        glm::vec3 fGravity = glm::vec3(0.0f, GRAVITY, 0.0f) * pi.density;

        for (const auto& pj : ps) {
            if (&pi == &pj) continue;
            glm::vec3 rij = pi.position - pj.position;
            float r = glm::length(rij);
            fPressure += -MASS * (pi.pressure + pj.pressure) / (2.0f * pj.density) * spikyGrad(rij);
            fViscosity += VISCOSITY * MASS * (pj.velocity - pi.velocity) / pj.density * viscLaplacian(r);
        }

        pi.force = fPressure + fViscosity + fGravity;
    }
}

void integrate(std::vector<Particle>& ps) {
    glm::vec3 boundsMin(0.0f), boundsMax(1.0f);
    for (auto& p : ps) {
        glm::vec3 a = p.force / p.density;
        p.velocity += a * TIME_STEP;
        p.position += p.velocity * TIME_STEP;

        for (int i = 0; i < 3; ++i) {
            if (p.position[i] < boundsMin[i]) {
                p.position[i] = boundsMin[i];
                p.velocity[i] *= -0.5f;
            } else if (p.position[i] > boundsMax[i]) {
                p.position[i] = boundsMax[i];
                p.velocity[i] *= -0.5f;
            }
        }
    }
}

// ----- MAIN -----
int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "SPH Fluid", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    GLuint shader = createShaderProgram();
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    std::vector<Particle> particles = initParticles(10, 10, 10, 0.05f);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window)) {
        computeDensityPressure(particles);
        computeForces(particles);
        integrate(particles);

        std::vector<glm::vec3> positions;
        for (const auto& p : particles) positions.push_back(p.position);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(glm::vec3), positions.data());

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 view = glm::lookAt(glm::vec3(0.5f, 0.5f, 2.0f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.01f, 100.0f);

        GLint viewLoc = glGetUniformLocation(shader, "view");
        GLint projLoc = glGetUniformLocation(shader, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, (GLsizei)positions.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
