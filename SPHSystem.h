#pragma once
#include "Particle.h"
#include "SPHParameters.h"
#include <vector>

class SPHSystem {
public:
    std::vector<Particle> particles;

    SPHSystem();
    ~SPHSystem();

    void computeDensityPressure();
    void computeForces();
    void integrate();

private:
    void initializeParticles();
};
