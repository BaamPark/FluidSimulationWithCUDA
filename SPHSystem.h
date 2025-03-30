#pragma once
#include "Particle.h"
#include "SPHParameters.h"
#include <vector>

class SPHSystem {
public:
    std::vector<Particle> particles;

    SPHSystem(int numX, int numY, int numZ, float spacing);

    void computeDensityPressure();
    void computeForces();
    void integrate();

private:
    void initializeParticles(int numX, int numY, int numZ, float spacing);
};
