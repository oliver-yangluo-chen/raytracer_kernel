#pragma once

#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <thrust/device_ptr.h>

#include "Camera.h"
#include "World.h"
#include "FrameBuffer.h"

//----------------------------------------
// RenderKernel: encapsulates CUDA resources
// for scene setup and ray-tracing pipeline
//----------------------------------------
class RenderKernel {
public:
    //--- Constructors & Destructor ---
    RenderKernel() = default;
    RenderKernel(cudaGraphicsResource_t gfxRes, int width, int height);
    ~RenderKernel();

    //--- Scene & Camera Operations ---
    void updateCamera(const glm::vec3& position,
                      const glm::vec3& direction,
                      const glm::vec3& upVector);

    //--- Rendering Lifecycle ---
    void renderFrame();
    void updateSize(int newWidth, int newHeight);

private:
    // Device pointers to scene objects and RNG states
    thrust::device_ptr<Camera*>      deviceCamera_;
    thrust::device_ptr<curandState>  rngStates_;
    thrust::device_ptr<World*>       deviceWorld_;

    // Graphics resource handle for frame buffer mapping
    cudaGraphicsResource_t           gfxResource_;

    // Host-side camera settings used for device init
    CameraInfo                       camInfo_;

    // Frame buffer (host-managed, device-mapped during render)
    FrameBuffer*                     framebuf_;

    // Current render dimensions
    int                              width_  = 0;
    int                              height_ = 0;
};
