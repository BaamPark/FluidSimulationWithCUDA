## Contribution:
- Beomseok Park.: Implement both a CPU-based SPH simulation and a GPU-based SPH simulation.
- Shutong Peng: Implement Particle Shader

## Note:
- The GPU-based SPH simulation uses CUDA global memory only.
- We implement SPHsystem using CUDA shared memory however, we haven't found performance boost compared to CUDA global memory one.

## Blog
- https://baampark.github.io/posts/2025-04-06_sph/

```bash
g++ main.cpp ParticleRenderer.cpp SPHSystem.cpp shader.cpp -o fluid_sim -lGL -lGLEW -lglfw
```


```bash
nvcc -arch=sm_86 -std=c++17 \
     Shader.cpp ParticleRenderer.cpp main_cuda.cpp SPHSystemCUDA.cu \
     -lglfw -lGLEW -lGL -o fluid_sim_cuda


# nvcc -arch=sm_86 -std=c++17 \
#      Shader.cpp ParticleRenderer.cpp main_cuda.cpp SPHSytemSharedCUDA.cu \
#      -lglfw -lGLEW -lGL -o fluid_sim_shared_cuda
```