#pragma once

const int numX = 10;
const int numY = 10;
const int numZ = 10;
const float spacing = 0.01f;
const float REST_DENSITY = 1000.0f; // ↑ Higher rest density → stronger pressure response to compression
const float GAS_CONSTANT = 200.0f; // ↑ Higher → more aggressive pressure forces → jittery, unstable fluid if too high
const float VISCOSITY = 3.5f; // ↑ Higher → thicker fluid (like honey) → smooths out velocity differences
const float TIME_STEP = 0.0005f;
const float MASS = 0.001f; // ↑ Higher mass → increases density and pressure → may cause instability if too high
const float SMOOTHING_RADIUS = 0.02f; // ↑ Larger radius → smoother fluid, more neighbors, slower computation
const float GRAVITY = -9.81f;
const float DAMPING = -0.3f;