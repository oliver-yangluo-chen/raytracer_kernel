#include "Window.h"
#include <iostream>
#include <memory>
#include <iomanip>

//-----------------------------------------------------------------------------
// Constructor / Destructor
//-----------------------------------------------------------------------------
Window::Window(unsigned w, unsigned h)
  : width_(w), height_(h), frameCount_(0)
{}

Window::~Window() {
    // Clean up CUDA-backed buffers
    currentFrame_->cuda_destroy();
    // Tear down GLFW
    glfwDestroyWindow(window_);
    glfwTerminate();
}

//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
bool Window::initialize() {
    if (!initGLFW())   return false;
    if (!initGLAD())   return false;
    if (!initFramebuffer()) return false;
    if (!initQuads())  return false;
    lastFrameTime_ = std::chrono::steady_clock::now();
    return true;
}

bool Window::initGLFW() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(width_, height_, "CUDA Ray Tracer", nullptr, nullptr);
    if (!window_) {
        std::cerr << "GLFW window creation failed\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSetWindowUserPointer(window_, this);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    return true;
}

bool Window::initGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    return true;
}

bool Window::initFramebuffer() {
    glViewport(0, 0, width_, height_);
    return true;
}

bool Window::initQuads() {
    // Fullscreen blit quad
    blitQuad_     = std::make_unique<Quad>(width_, height_);
    blitQuad_->make_FBO();

    // Shaders
    blitShader_   = std::make_unique<Shader>("./shaders/rendertype_screen.vert",
                                              "./shaders/rendertype_screen.frag");
    accumShader_  = std::make_unique<Shader>("./shaders/rendertype_accumulate.vert",
                                              "./shaders/rendertype_accumulate.frag");
    accumShader_->use();
    accumShader_->setInt("currentFrameTex", 0);
    accumShader_->setInt("lastFrameTex",    1);

    // Current & accumulated frames
    currentFrame_ = std::make_unique<Quad>(width_, height_);
    currentFrame_->cuda_init();
    currentFrame_->make_FBO();

    accumFrame_   = std::make_unique<Quad>(width_, height_);
    accumFrame_->make_FBO();

    return true;
}

//-----------------------------------------------------------------------------
// Main loop
//-----------------------------------------------------------------------------
int Window::run() {
    if (!initialize()) return -1;
    while (!glfwWindowShouldClose(window_)) {
        tick();
    }
    return 0;
}

void Window::tick() {
    auto now     = std::chrono::steady_clock::now();
    float delta  = std::chrono::duration<float, std::milli>(now - lastFrameTime_).count();
    lastFrameTime_ = now;

    handleInput(delta);
    renderFrame();
    frameCount_++;
}

//-----------------------------------------------------------------------------
// Input handling
//-----------------------------------------------------------------------------
void Window::handleInput(float dt) {
    input_.processQuit(window_);
    input_.processCamera(window_, *currentFrame_->_renderer, dt);
    if (input_.hasCameraMoved()) frameCount_ = 1;
}

//-----------------------------------------------------------------------------
// Rendering
//-----------------------------------------------------------------------------
void Window::renderFrame() {
    // Clear screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 1) Draw raytraced frame into texture
    glBindFramebuffer(GL_FRAMEBUFFER, currentFrame_->framebuffer);
    currentFrame_->render_kernel();
    blitShader_->use();
    currentFrame_->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2) Copy accumulation history
    blitFramebufferTexture(width_, height_,
                           accumFrame_->framebuffer, accumFrame_->texture,
                           blitQuad_->framebuffer,    blitQuad_->texture);

    // 3) Blend new frame into accumulation buffer
    glBindFramebuffer(GL_FRAMEBUFFER, accumFrame_->framebuffer);
    accumShader_->use();
    accumShader_->setInt("frameCount", frameCount_);
    currentFrame_->bindTexture(0);
    blitQuad_->bindTexture(1);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 4) Present accumulated result
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    blitShader_->use();
    glBindTexture(GL_TEXTURE_2D, accumFrame_->texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Swap and poll
    glfwSwapBuffers(window_);
    glfwPollEvents();
}

//-----------------------------------------------------------------------------
// GLFW callback (static)
//-----------------------------------------------------------------------------
void Window::framebufferResizeCallback(GLFWwindow* win, int w, int h) {
    auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(win));
    self->resize(w, h);
}

void Window::resize(unsigned w, unsigned h) {
    width_ = w;  height_ = h;
    glViewport(0, 0, w, h);
    currentFrame_->resize(w, h);
    accumFrame_->resize(w, h);
    blitQuad_   ->resize(w, h);
}

//-----------------------------------------------------------------------------
// FBO blit helper
//-----------------------------------------------------------------------------
void Window::blitFramebufferTexture(int w, int h,
                                    int srcFBO, unsigned srcTex,
                                    int dstFBO, unsigned dstTex)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFBO);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, srcTex, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFBO);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                           GL_TEXTURE_2D, dstTex, 0);
    glBlitFramebuffer(0,0,w,h, 0,0,w,h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    // unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
