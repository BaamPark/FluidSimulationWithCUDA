#pragma once
#include <cuda_runtime.h> 
#include <vector>
#include <glm/glm.hpp>
#include "particle.h"
#include "SPHParameters.h"

class SPHSystemCUDA {
public:
    std::vector<Particle> particles;               // still used by main.cpp

    SPHSystemCUDA();
    ~SPHSystemCUDA();

    void computeDensityPressure();                 // launch kernels
    void computeForces();
    void integrate();

private:
    int  N_;                                       // total particles
    void initializeParticles();

    // In global kernel, we cannot access particle directly 
    // because CUDA only supports low-level C++ objects like float or float3. 
    // So we need containers for particle.position
    float3 *d_pos_, *d_vel_, *d_force_;
    float  *d_density_, *d_pressure_;
};
