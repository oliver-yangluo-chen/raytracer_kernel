#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>

#include "Shader.h"
#include "Quad.h"
#include "Input.h"

//----------------------------------------------------------------------------- 
// Window: manages application lifecycle, input, and rendering loop
//----------------------------------------------------------------------------- 
class Window {
public:
    // Construct with initial dimensions
    Window(unsigned int width, unsigned int height);
    ~Window();

    // Initialize all subsystems; returns true on success
    bool initialize();

    // Main loop: process events until window should close
    int  run();

    // Resize viewport and associated framebuffers
    void resize(unsigned int newWidth, unsigned int newHeight);

private:
    // GLFW window handle
    GLFWwindow*          windowHandle_ = nullptr;

    // Screen dimensions
    unsigned int         width_        = 0;
    unsigned int         height_       = 0;

    // Frame timing
    int                  frameCount_   = 0;
    std::chrono::steady_clock::time_point lastFrameTime_;

    // Rendering resources
    std::unique_ptr<Shader> shader_;         // Screen blit shader
    std::unique_ptr<Shader> accumShader_;    // Accumulation shader
    std::unique_ptr<Quad>   blitQuad_;       // Fullscreen quad
    std::unique_ptr<Quad>   currentQuad_;    // Raytrace target
    std::unique_ptr<Quad>   accumQuad_;      // Accumulation target

    // Input handler
    Input                inputHandler_;

    // Subsystem init helpers
    bool initGLFW();
    bool initGLAD();
    bool initFramebuffers();
    bool initQuads();

    // Per-frame operations
    void processInput(float deltaTime);
    void renderFrame();
    void tick();

    // GLFW callback for window resizing
    static void framebufferResizeCallback(GLFWwindow* window, int w, int h);

    // FBO blit utility
    void blitTexture(int w, int h,
                     int srcFBO, unsigned int srcTex,
                     int dstFBO, unsigned int dstTex);
};
