// conclusion: the shared memory approach doesn't make improvement compared to global memory approach. It could be flawed code or the nature of SPH.

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
int id = blockIdx.x*blockDim.x + threadIdx.x; //block_id * num_thread_per_block + thread_id

if (id>=N) return;

float density = 0.f;
float3 pi = pos[id];

for (int j=0; j<N; ++j) {
    float3 rij{
        pos[j].x - pi.x,
        pos[j].y - pi.y,
        pos[j].z - pi.z};
    float r2 = rij.x*rij.x + rij.y*rij.y + rij.z*rij.z;
    density += MASS * poly6Kernel(r2, SMOOTHING_RADIUS);
}
dens[id] = density;
pres[id] = GAS_CONSTANT * (density - REST_DENSITY);
}

__global__ void densityPressureKernel_shared(
    int N, const float3* pos, float* dens, float* pres)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    float3 pi = pos[i];
    float density = 0.0f;

    // Shared memory tile
    __shared__ float3 shPos[256]; //CUDA doesn't allow shPos[blockDim.x];

    int tileSize = blockDim.x;  //256
    int numTiles = (N + tileSize - 1) / tileSize; //if #particle is 1,000, #tiles is 4

    for (int tile = 0; tile < numTiles; ++tile) {
        int j = tile * blockDim.x + threadIdx.x; //Thread_id3: 0*256 + 3, 1*256 + 3, 2*256 + 3, 3*256 + 3

        // Load particle j into shared memory
        if (j < N)
            shPos[threadIdx.x] = pos[j]; //overwriting shPos? 

        __syncthreads(); //avoid race condition: Some threads may start reading shPos[k] before others have finished writing to it
        //at this point, shPos will be filled with all particles
        
        // Iterate over particles within the current tile
        for (int k = 0; k < blockDim.x; ++k) {
            int actualIdx = tile * blockDim.x + k;
            if (actualIdx >= N) break;

            float3 rij;
            rij.x = shPos[k].x - pi.x;
            rij.y = shPos[k].y - pi.y;
            rij.z = shPos[k].z - pi.z;

            float r2 = rij.x * rij.x + rij.y * rij.y + rij.z * rij.z;
            density += MASS * poly6Kernel(r2, SMOOTHING_RADIUS);
        }

        __syncthreads(); //avoid race condition: prevent threads from overwriting shPos
    }

    dens[i] = density;
    pres[i] = GAS_CONSTANT * (density - REST_DENSITY);
}

//this kernel doesn't work as expected. 
// 1) can see particles in the very first frame but then disappear 
// 2) the value shDens NaN, which means not loaded correctly. 
// 3) when removing shDens, particles move but velocity looks different. 
__global__ void forceKernel_shared(
    int N,
    const float3* pos, const float3* vel,
    const float* dens, const float* pres,
    float3* force)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    float3 fi = make_float3(0, 0, 0);
    float3 pi = pos[i];
    float3 vi = vel[i];
    float  di = dens[i];
    float  pi_pres = pres[i];

    // Shared memory
    __shared__ float3 shPos[256];
    __shared__ float3 shVel[256];
    __shared__ float  shDens[256];
    __shared__ float  shPres[256];

    int tileSize = blockDim.x;
    int numTiles = (N + tileSize - 1) / tileSize;

    for (int tile = 0; tile < numTiles; ++tile) {
        int j = tile * tileSize + threadIdx.x;

        if (j < N) {
            shPos[threadIdx.x]  = pos[j];
            shVel[threadIdx.x]  = vel[j];
            shDens[threadIdx.x] = dens[j];
            shPres[threadIdx.x] = pres[j];
        }

        __syncthreads();

        for (int k = 0; k < tileSize; ++k) {
            int jIdx = tile * tileSize + k;
            if (jIdx >= N || jIdx == i) continue;

            float3 rij = {
                pi.x - shPos[k].x,
                pi.y - shPos[k].y,
                pi.z - shPos[k].z
            };

            float r2 = rij.x * rij.x + rij.y * rij.y + rij.z * rij.z;
            float r  = sqrtf(fmaxf(r2, 1e-12f));  // avoid sqrt(0)

            // Pressure force
            float3 grad = spikyGradient(rij, SMOOTHING_RADIUS);
            // float pressureScale = -MASS * (pi_pres + shPres[k]) / (2.0f * dens[jIdx]);
            float pressureScale = -MASS * (pi_pres + shPres[k]) / (2.0f * shDens[k]);
            fi.x += pressureScale * grad.x;
            fi.y += pressureScale * grad.y;
            fi.z += pressureScale * grad.z;

            // Viscosity force
            float lap = viscosityLaplacian(r, SMOOTHING_RADIUS);
            float3 vDiff = {
                shVel[k].x - vi.x,
                shVel[k].y - vi.y,
                shVel[k].z - vi.z
            };
            float viscosityScale = VISCOSITY * MASS / shDens[k];
            // float viscosityScale = VISCOSITY * MASS / dens[jIdx];
            fi.x += viscosityScale * vDiff.x * lap;
            fi.y += viscosityScale * vDiff.y * lap;
            fi.z += viscosityScale * vDiff.z * lap;
        }

        __syncthreads();  // ensure reads are done before loading next tile
    }

    // Gravity
    fi.y += di * GRAVITY;
    force[i] = fi;
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

__global__ void integrateKernel_shared(
    int N, float3* pos, float3* vel,
    const float3* force, const float* dens)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    // Fixed-size shared memory for current thread's force and density
    __shared__ float3 shForce[256];
    __shared__ float  shDens[256];

    // Load per-thread values into shared memory
    shForce[threadIdx.x] = force[i];
    shDens[threadIdx.x] = dens[i];

    __syncthreads();  // ensure values are available to threadIdx.x

    float3 v = vel[i];
    float3 f = shForce[threadIdx.x];
    float  invMass = 1.f / shDens[threadIdx.x];

    // Semi-implicit Euler integration
    v.x += f.x * invMass * TIME_STEP;
    v.y += f.y * invMass * TIME_STEP;
    v.z += f.z * invMass * TIME_STEP;

    float3 p = pos[i];
    p.x += v.x * TIME_STEP;
    p.y += v.y * TIME_STEP;
    p.z += v.z * TIME_STEP;

    // Bounding box [0,1]×[0,0.5]×[0,1]
    const float3 bmin{0, 0, 0}, bmax{1, 0.5, 1};
    if (p.x < bmin.x) { p.x = bmin.x; v.x *= DAMPING; }
    if (p.x > bmax.x) { p.x = bmax.x; v.x *= DAMPING; }
    if (p.y < bmin.y) { p.y = bmin.y; v.y *= DAMPING; }
    if (p.y > bmax.y) { p.y = bmax.y; v.y *= DAMPING; }
    if (p.z < bmin.z) { p.z = bmin.z; v.z *= DAMPING; }
    if (p.z > bmax.z) { p.z = bmax.z; v.z *= DAMPING; }

    pos[i] = p;
    vel[i] = v;
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

//helper function designed to calculate the necessary grid dimension (gridDim)
inline dim3 gridFor(int N,int block){ return dim3((N+block-1)/block); }

void SPHSystemCUDA::computeDensityPressure(){
    densityPressureKernel_shared<<<gridFor(N_,256),256>>>(N_,d_pos_,d_density_,d_pressure_);
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
