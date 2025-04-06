https://chatgpt.com/share/67e8401f-df2c-800e-9caf-1a0c904e5bfa

## Milestons
- Week1: CPU-based SPH implementation with simple shading
- Week2: Support lighting (phong) using sphere impostors

## Terminology
- Sphere imposter: 2D quad (a square) that always faces the camera
- 


## Ref.
- https://www.youtube.com/watch?v=rSKMYc1CQHE
- https://www.cs.cmu.edu/~scoros/cs15467-s16/lectures/11-fluids2.pdf
- https://bgolus.medium.com/rendering-a-sphere-on-a-quad-13c92025570c
- https://www.fbxiang.com/blog/2019/05/01/smoothed-particle-hydrodynamics.html

```bash
g++ main.cpp ParticleRenderer.cpp SPHSystem.cpp shader.cpp -o fluid_sim -lGL -lGLEW -lglfw
```