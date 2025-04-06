//Ref https://www.cs.cmu.edu/~scoros/cs15467-s16/lectures/11-fluids2.pdf

#include "SPHSystem.h"

SPHSystem::SPHSystem(int numX, int numY, int numZ, float spacing) {
    initializeParticles(numX, numY, numZ, spacing);
}

//This will create a grid of particles. For example, calling SPHSystem system(10, 10, 10, 0.05f); creates 1000 particles spaced at 0.05 units apart.
void SPHSystem::initializeParticles(int numX, int numY, int numZ, float spacing) {
    for (int x = 0; x < numX; ++x) {
        for (int y = 0; y < numY; ++y) {
            for (int z = 0; z < numZ; ++z) {
                glm::vec3 pos = glm::vec3(
                    x * spacing,
                    y * spacing,
                    z * spacing
                );
                particles.emplace_back(pos);
            }
        }
    }
}

//helper function for computeDensityPressure
float poly6Kernel(float rSquared, float h) {
    const float h2 = h * h;
    if (rSquared >= h2) return 0.0f;

    float diff = h2 - rSquared;
    float coeff = 315.0f / (64.0f * M_PI * pow(h, 9));
    return coeff * diff * diff * diff;
}

//Density and Pressure Calculation; O(n^2)
void SPHSystem::computeDensityPressure() {
    for (auto& pi : particles) {
        pi.density = 0.0f;
        for (const auto& pj : particles) {
            glm::vec3 rij = pj.position - pi.position;
            float r2 = glm::dot(rij, rij);
            pi.density += MASS * poly6Kernel(r2, SMOOTHING_RADIUS);
        }
        pi.pressure = GAS_CONSTANT * (pi.density - REST_DENSITY);
    }
}


// helper function used for pressureForce in computeForces function
glm::vec3 spikyGradient(const glm::vec3& rij, float h) {
    float r = glm::length(rij);
    if (r == 0.0f || r > h) return glm::vec3(0.0f);

    float coeff = -45.0f / (M_PI * pow(h, 6));
    return static_cast<float>(coeff * pow(h - r, 2)) * (rij / static_cast<float>(r));
}

// helper function used for viscosity in computeForces function
float viscosityLaplacian(float r, float h) {
    if (r >= h) return 0.0f;
    float coeff = 45.0f / (M_PI * pow(h, 6));
    return coeff * (h - r);
}

void SPHSystem::computeForces() {
    for (auto& pi : particles) {
        glm::vec3 pressureForce(0.0f);
        glm::vec3 viscosityForce(0.0f);
        glm::vec3 gravityForce = glm::vec3(0.0f, GRAVITY, 0.0f) * pi.density;

        for (const auto& pj : particles) {
            if (&pi == &pj) continue;

            glm::vec3 rij = pi.position - pj.position;
            float r = glm::length(rij);

            // Pressure force
            glm::vec3 gradW = spikyGradient(rij, SMOOTHING_RADIUS);
            pressureForce += -MASS * (pi.pressure + pj.pressure) / (2.0f * pj.density) * gradW;

            // Viscosity force
            float laplacianW = viscosityLaplacian(r, SMOOTHING_RADIUS);
            viscosityForce += VISCOSITY * MASS * (pj.velocity - pi.velocity) / pj.density * laplacianW;
        }

        pi.force = pressureForce + viscosityForce + gravityForce;
    }
}

// refer to the slide 27
// Time Integration (Euler Method) is the process of computing new positions and velocities for the next step
void SPHSystem::integrate() {
    //Defines the simulation bounding box: all particles must stay within [0, 1] along x, y, and z
    const glm::vec3 boundsMin(0.0f);
    const glm::vec3 boundsMax(1.0f); // You can tweak this

    for (auto& p : particles) {
        // Acceleration
        glm::vec3 acceleration = p.force / p.density;

        //Semi-implicit Euler
        p.velocity += acceleration * TIME_STEP; // velocity update
        p.position += p.velocity * TIME_STEP; // position update

        // Simple boundary constraint that particles stay within a defined simulation box
        for (int i = 0; i < 3; ++i) {
            if (p.position[i] < boundsMin[i]) {
                p.position[i] = boundsMin[i];
                p.velocity[i] *= DAMPING;
            } else if (p.position[i] > boundsMax[i]) {
                p.position[i] = boundsMax[i];
                p.velocity[i] *= DAMPING;
            }
        }
    }
}

//Simulation Loop
// system.computeDensityPressure();
// system.computeForces();
// system.integrate();