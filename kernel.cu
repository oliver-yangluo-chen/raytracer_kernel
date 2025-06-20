#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <iostream>
#include <glm/glm.hpp>
#include <curand_kernel.h>
#include <thrust/device_new.h>
#include <thrust/device_free.h>

#include "Window.h"
#include "cuda_errors.h"
#include "FrameBuffer.h"
#include "Ray.h"
#include "Sphere.h"
#include "World.h"
#include "Camera.h"
#include "BVHNode.h"
#include "raytracer/kernel.h"

//–– GPU kernels ––

// Initialize per-pixel RNG states
__global__ void initRng(curandState* rngStates, int resX, int resY) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= resX || y >= resY) return;
    int idx = y * resX + x;
    curand_init(1984, idx, 0, &rngStates[idx]);
}

// Build scene and camera once
__global__ void setupScene(World** worldPtr, Camera** camPtr, CameraInfo camInfo) {
    if (threadIdx.x != 0 || blockIdx.x != 0) return;
    World* w = new World();
    w->add(new Sphere({0, 0, -1}, 0.5f, new Lambertian({0.8f,0.3f,0.3f})));
    w->add(new Sphere({-1.01f, 0, -1}, 0.5f, new Dielectric(1.5f)));
    w->add(new Sphere({-1, 10, -1}, 0.5f, new Dielectric(1.5f)));
    w->add(new Sphere({1, 0, -1}, 0.5f, new Metal({0.8f,0.8f,0.8f}, 0.3f)));
    w->add(new Sphere({0, -1000.5f, 0}, 1000.0f,
        new Lambertian(new CheckerTexture({0.2f,0.3f,0.1f},{0.9f}))));
    *worldPtr = w;
    *camPtr   = camInfo.construct_camera();
}

// Trace rays and write pixels
__global__ void traceRays(FrameBuffer fb, World** worldPtr, Camera** camPtr, curandState* rngStates) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= fb.width || y >= fb.height) return;
    int idx = y * fb.width + x;

    curandState rng = rngStates[idx];
    glm::vec3 accumColor{0.0f};
    const int samples = 3;

    for (int s = 0; s < samples; ++s) {
        float u = (x + curand_uniform(&rng)) / float(fb.width);
        float v = (y + curand_uniform(&rng)) / float(fb.height);
        Ray ray = (*camPtr)->get_ray(u, v);
        accumColor += fb.color(ray, **worldPtr, &rng);
    }
    rngStates[idx] = rng;

    accumColor /= float(samples);
    accumColor = glm::sqrt(accumColor);  // gamma correction
    fb.writePixel(x, y, glm::vec4(accumColor, 1.0f));
}

// Clean up device objects
__global__ void cleanupScene(World** worldPtr, Camera** camPtr) {
    delete *worldPtr;
    delete *camPtr;
}

//–– Host-side RenderKernel class ––

KernelInfo::KernelInfo(cudaGraphicsResource_t gfxRes, int width, int height)
    : resources(gfxRes), resX(width), resY(height)
{
    fb = new FrameBuffer(resX, resY);
    camInfo = CameraInfo({0,0,0},{0,0,0},90.0f,float(resX),float(resY));

    // allocate on device
    thrust::device_new<Camera*   >(&devCamera);
    thrust::device_new<curandState>(resX*resY, &rngStates);
    thrust::device_new<World*    >(&devWorld);

    // build scene & camera
    setupScene<<<1,1>>>(&devWorld, &devCamera, camInfo);
    check_cuda_errors(cudaDeviceSynchronize());

    // init RNG
    dim3 block{8,8};
    dim3 grid{(resX+7)/8,(resY+7)/8};
    initRng<<<grid,block>>>(rngStates, resX, resY);
    check_cuda_errors(cudaDeviceSynchronize());
}

void KernelInfo::resize(int width, int height) {
    resX = width; resY = height;
    delete fb;
    fb = new FrameBuffer(resX, resY);

    thrust::device_free(rngStates);
    thrust::device_new<curandState>(resX*resY, &rngStates);

    dim3 block{8,8}, grid{(resX+7)/8,(resY+7)/8};
    initRng<<<grid,block>>>(rngStates, resX, resY);
    check_cuda_errors(cudaDeviceSynchronize());
}

void KernelInfo::set_camera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up) {
    // single-thread update
    set_device_camera<<<1,1>>>(&devCamera, pos, dir, up, float(resX)/resY);
    check_cuda_errors(cudaDeviceSynchronize());
}

void KernelInfo::render() {
    check_cuda_errors(cudaGraphicsMapResources(1,&resources));
    check_cuda_errors(cudaGraphicsResourceGetMappedPointer(
        (void**)&fb->device_ptr, &fb->buffer_size, resources));

    dim3 block{32,
