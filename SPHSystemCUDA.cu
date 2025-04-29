#include "SPHSystemCUDA.cuh"
#include <cuda_runtime.h>
#include <stdexcept>
#include <cstdlib>
#include <ctime>

// ---------- device‑side math helpers (same formulas as CPU) ------------------
__device__ float poly6Kernel(float r2, float h) {
    float h2 = h*h;
    if (r2 >= h2) return 0.f;
    float diff = h2 - r2;
    float coeff = 315.f / (64.f * M_PI * powf(h, 9));
    return coeff * diff * diff * diff;
}
__device__ float viscosityLaplacian(float r, float h){
    return (r>=h)?0.f : 45.f/(M_PI*powf(h,6))*(h-r);
}
__device__ float3 spikyGradient(float3 rij,float h){
    float r = sqrtf(rij.x*rij.x + rij.y*rij.y + rij.z*rij.z);
    if (r==0.f || r>h) return make_float3(0,0,0);
    float coeff = -45.f/(M_PI*powf(h,6))*powf(h-r,2)/r;
    return make_float3(coeff*rij.x, coeff*rij.y, coeff*rij.z);
}

// ---------- kernels ----------------------------------------------------------
__global__ void densityPressureKernel(
        int N, const float3* pos, float* dens, float* pres)
{
    int i = blockIdx.x*blockDim.x + threadIdx.x;
    if (i>=N) return;

    float density = 0.f;
    float3 pi = pos[i];

    for (int j=0; j<N; ++j) {
        float3 rij{
            pos[j].x - pi.x,
            pos[j].y - pi.y,
            pos[j].z - pi.z};
        float r2 = rij.x*rij.x + rij.y*rij.y + rij.z*rij.z;
        density += MASS * poly6Kernel(r2, SMOOTHING_RADIUS);
    }
    dens[i] = density;
    pres[i] = GAS_CONSTANT * (density - REST_DENSITY);
}

__global__ void forceKernel(
        int N,const float3* pos,const float3* vel,
        const float* dens,const float* pres,
        float3* force)
{
    int i = blockIdx.x*blockDim.x + threadIdx.x;
    if (i>=N) return;

    float3 fi = make_float3(0,0,0);
    float3 pi = pos[i];
    float3 vi = vel[i];
    float  di = dens[i];
    float  pi_pres = pres[i];

    for (int j=0;j<N;++j){
        if (i==j) continue;
        float3 rij{
            pi.x - pos[j].x,
            pi.y - pos[j].y,
            pi.z - pos[j].z};
        float  r  = sqrtf(rij.x*rij.x + rij.y*rij.y + rij.z*rij.z);

        // pressure
        float3 grad = spikyGradient(rij, SMOOTHING_RADIUS);
        fi.x += -MASS*(pi_pres+pres[j])/(2.f*dens[j])*grad.x;
        fi.y += -MASS*(pi_pres+pres[j])/(2.f*dens[j])*grad.y;
        fi.z += -MASS*(pi_pres+pres[j])/(2.f*dens[j])*grad.z;

        // viscosity
        float lap = viscosityLaplacian(r, SMOOTHING_RADIUS);
        fi.x += VISCOSITY*MASS*(vel[j].x-vi.x)/dens[j]*lap;
        fi.y += VISCOSITY*MASS*(vel[j].y-vi.y)/dens[j]*lap;
        fi.z += VISCOSITY*MASS*(vel[j].z-vi.z)/dens[j]*lap;
    }
    // gravity
    fi.y += di*GRAVITY;

    force[i]=fi;
}

__global__ void integrateKernel(
        int N,float3* pos,float3* vel,const float3* force,const float* dens)
{
    int i = blockIdx.x*blockDim.x + threadIdx.x;
    if (i>=N) return;

    // semi‑implicit Euler
    float3 v = vel[i];
    float3 f = force[i];
    float  invMass = 1.f/dens[i];   // actually 1/ρ not mass
    v.x += f.x*invMass*TIME_STEP;
    v.y += f.y*invMass*TIME_STEP;
    v.z += f.z*invMass*TIME_STEP;

    float3 p = pos[i];
    p.x += v.x*TIME_STEP;
    p.y += v.y*TIME_STEP;
    p.z += v.z*TIME_STEP;

    // bounding box [0,1]×[0,0.5]×[0,1]
    const float3 bmin{0,0,0}, bmax{1,0.5,1};
    if(p.x<bmin.x){p.x=bmin.x; v.x*=DAMPING;}
    if(p.x>bmax.x){p.x=bmax.x; v.x*=DAMPING;}
    if(p.y<bmin.y){p.y=bmin.y; v.y*=DAMPING;}
    if(p.y>bmax.y){p.y=bmax.y; v.y*=DAMPING;}
    if(p.z<bmin.z){p.z=bmin.z; v.z*=DAMPING;}
    if(p.z>bmax.z){p.z=bmax.z; v.z*=DAMPING;}

    pos[i]=p; vel[i]=v;
}

// ---------- host‑side constructor / destructor ------------------------------
SPHSystemCUDA::SPHSystemCUDA()
{
    initializeParticles();
    N_ = static_cast<int>(particles.size());

    // allocate & copy to device
    size_t v3 = N_*sizeof(float3), v1 = N_*sizeof(float);
    cudaMalloc(&d_pos_, v3);
    cudaMalloc(&d_vel_, v3);
    cudaMalloc(&d_force_, v3);
    cudaMalloc(&d_density_, v1);
    cudaMalloc(&d_pressure_, v1);

    std::vector<float3> h_pos(N_), h_vel(N_);
    for(int i=0;i<N_;++i){
        h_pos[i] = make_float3(particles[i].position.x,
                               particles[i].position.y,
                               particles[i].position.z);
        h_vel[i] = make_float3(0,0,0);
    }
    cudaMemcpy(d_pos_, h_pos.data(), v3, cudaMemcpyHostToDevice);
    cudaMemcpy(d_vel_, h_vel.data(), v3, cudaMemcpyHostToDevice);
}

SPHSystemCUDA::~SPHSystemCUDA(){
    cudaFree(d_pos_); cudaFree(d_vel_); cudaFree(d_force_);
    cudaFree(d_density_); cudaFree(d_pressure_);
}

// Initialize particles in a noisy grid (mirroring CPU version)
void SPHSystemCUDA::initializeParticles() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    float noiseScale = spacing * 0.1f;
    for (int x = 0; x < numX; ++x) {
        for (int y = 0; y < numY; ++y) {
            for (int z = 0; z < numZ; ++z) {
                float nx = ((std::rand() % 1000) / 1000.0f - 0.5f) * noiseScale;
                float ny = ((std::rand() % 1000) / 1000.0f - 0.5f) * noiseScale;
                float nz = ((std::rand() % 1000) / 1000.0f - 0.5f) * noiseScale;
                glm::vec3 pos(
                    x * spacing + nx,
                    y * spacing + 0.5f + ny,
                    z * spacing + nz
                );
                particles.emplace_back(pos);
            }
        }
    }
}

// ---------- public API: launch kernels & copy back for renderer -------------
inline dim3 gridFor(int N,int block){ return dim3((N+block-1)/block); }

void SPHSystemCUDA::computeDensityPressure(){
    densityPressureKernel<<<gridFor(N_,256),256>>>(N_,d_pos_,d_density_,d_pressure_);
    cudaDeviceSynchronize();
}

void SPHSystemCUDA::computeForces(){
    forceKernel<<<gridFor(N_,256),256>>>(N_,d_pos_,d_vel_,d_density_,d_pressure_,d_force_);
    cudaDeviceSynchronize();
}

void SPHSystemCUDA::integrate(){
    integrateKernel<<<gridFor(N_,256),256>>>(N_,d_pos_,d_vel_,d_force_,d_density_);
    cudaDeviceSynchronize();

    // copy positions back so ParticleRenderer can update VBO
    std::vector<float3> h_pos(N_);
    cudaMemcpy(h_pos.data(), d_pos_, N_*sizeof(float3), cudaMemcpyDeviceToHost);

    for (int i=0;i<N_;++i){
        particles[i].position = glm::vec3(h_pos[i].x,h_pos[i].y,h_pos[i].z);
    }
}
