#pragma once
#include <cuda_runtime.h>
#include <vector>
#include <glm/glm.hpp>
#include "particle.h"
#include "SPHParameters.h"

class SPHSystemSharedCUDA {
public:
    std::vector<Particle> particles;

    SPHSystemSharedCUDA();
    ~SPHSystemSharedCUDA();

    void computeDensityPressure();
    void computeForces();
    void integrate();

private:
    int  N_;
    void initializeParticles();

    float3 *d_pos_, *d_vel_, *d_force_;
    float  *d_density_, *d_pressure_;
};
