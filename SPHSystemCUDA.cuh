#pragma once
#include <cuda_runtime.h> 
#include <vector>
#include <glm/glm.hpp>
#include "particle.h"
#include "SPHParameters.h"

class SPHSystemCUDA {
public:
    std::vector<Particle> particles;               // still used by main.cpp

    SPHSystemCUDA(int nx, int ny, int nz, float spacing);
    ~SPHSystemCUDA();

    void computeDensityPressure();                 // launch kernels
    void computeForces();
    void integrate();

private:
    int  N_;                                       // total particles
    void initializeParticles(int nx,int ny,int nz,float spacing);

    // device pointers (SoA for coalescing)
    float3 *d_pos_, *d_vel_, *d_force_;
    float  *d_density_, *d_pressure_;
};
