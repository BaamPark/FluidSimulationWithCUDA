## Milestons
- B.P.
    - CPU-based SPH implementation with simple shading
    - tune parameters
    - parallelize computation for density, pressure, forces using CUDA
- S.P
    - Make the simulation interactive with user (change camera angle)
    - Support lighting for particles

## Terminology
- Density: the density of a particle is computed based on the mass and distance to neighboring particles within the smoothing radius
- Pressure: the force exerted per unit area; derived from density 

## Lighting options
- Marching Cubes
- 

## Math and Algorithm
- https://baampark.github.io/posts/2025-04-06_sph/

## Notes
### 2025-04-06
- Overshoot problems -> tune time stamp under CFL Condition


## Ref.
- https://www.youtube.com/watch?v=rSKMYc1CQHE
- https://www.cs.cmu.edu/~scoros/cs15467-s16/lectures/11-fluids2.pdf
- https://bgolus.medium.com/rendering-a-sphere-on-a-quad-13c92025570c
- https://www.fbxiang.com/blog/2019/05/01/smoothed-particle-hydrodynamics.html

```bash
g++ main.cpp ParticleRenderer.cpp SPHSystem.cpp shader.cpp -o fluid_sim -lGL -lGLEW -lglfw
```