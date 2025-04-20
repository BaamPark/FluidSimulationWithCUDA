## Milestons
- B.P.
    - CPU-based SPH implementation with simple shading
    - tune parameters
    - parallelize computation for density, pressure, forces using CUDA
    - average or sum the Delta t where
        - t_1: pre-simulation, t_2: post-simulation, t_3: post-rendering
- S.P
    - Make the simulation interactive with user (change camera angle)
    - Support lighting for particles

## Terminology
- Density: the density of a particle is computed based on the mass and distance to neighboring particles within the smoothing radius
- Pressure: the force exerted per unit area; derived from density 

## Lighting options
- Marching Cubes
- Sphere impostor

## Blog
- https://baampark.github.io/posts/2025-04-06_sph/


## Ref.
- https://www.youtube.com/watch?v=rSKMYc1CQHE
- https://www.cs.cmu.edu/~scoros/cs15467-s16/lectures/11-fluids2.pdf
- https://bgolus.medium.com/rendering-a-sphere-on-a-quad-13c92025570c
- https://www.fbxiang.com/blog/2019/05/01/smoothed-particle-hydrodynamics.html

```bash
g++ main.cpp ParticleRenderer.cpp SPHSystem.cpp shader.cpp -o fluid_sim -lGL -lGLEW -lglfw
```

```bash
# Build the GPU-accelerated version with CUDA (requires NVIDIA GPU & CUDA Toolkit)
# Defines USE_CUDA to select the CUDA backend in main_cuda.cpp,
# and compiles/links SPHSystemCUDA (stubs) together with rendering code.
nvcc -arch=sm_86 -std=c++17 \
     Shader.cpp ParticleRenderer.cpp main_cuda.cpp SPHSystemCUDA.cu \
     -lglfw -lGLEW -lGL -o fluid_sim_cuda
```