#pragma once

#define GLFW_INCLUDE_VULKAN

#include <BRASBuilder.h>
#include <BRAppState.h>
#include <BRCameraManip.h>
#include <BRCommandPool.h>
#include <BRDescMgr.h>
#include <BRDevice.h>
#include <BRFramebuffer.h>
#include <BRInstance.h>
#include <BRMemoryMgr.h>
#include <BRModelManip.h>
#include <BRRaster.h>
#include <BRRayTracer.h>
#include <BRRenderPass.h>
#include <BRScene.h>
#include <BRSurface.h>
#include <BRSwapchain.h>
#include <BRSyncMgr.h>
#include <GLFW/glfw3.h>

#include <array>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace BR
{

class BRRender
{
   public:
    BRRender();
    void run();

    bool m_framebufferResized = false;

    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 cameraPos;
        int iteration;
        bool accumulate;
        int mode;
    };

   private:
    GLFWwindow* m_window;

    Raster m_raster;
    RayTracer m_raytracer;

    CommandPool m_commandPool;

    DescMgr& m_descMgr;
    SyncMgr& m_syncMgr;
    MemoryMgr& m_bufferAlloc;

    int m_framesInFlight;

    ModelManip m_modelManip;
    CameraManip m_cameraManip;

    vk::Device m_device;

    std::vector<vk::CommandBuffer> m_commandBuffers;
    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;

    Scene m_scene;

    RenderPass m_renderPass;

    vk::DescriptorPool m_descriptorPool;

    std::vector<vk::Buffer> m_uniformBuffers;

    int m_currentFrame = 0;

    bool m_rtMode = false;
    int m_transformMode = 0;
    bool m_modelInput = false;
    bool m_camInput = false;
    int m_iteration = 0;
    bool m_rtAccumulate = true;
    int m_rtType = 0;

    void initWindow();
    void initVulkan();
    void loadModel( std::string name );
    void recreateSwapchain();
    void createUIRenderPass();

    void mainLoop();
    void drawFrame();
    void cleanup();

    void initUI();
    void drawUI();

    void updateUniformBuffer( uint32_t currentImage );

    void onMouseButton( int button, int action, int mods );
    void onMouseMove( int x, int y );
};

}  // namespace BR

/*
    Swap chain gives you the images (VRAM)
    Create image views for these images
    Describe what you're planning to do with the memory - render pass - I want to draw into the image
    Describe the graphics pipeline 
    Bind the image views from the swap chain to the render pass attachment - framebuffer    
    Create the command pool and command buffer
    Record your draw commands to the command buffer
    Submit your buffer, synchronizing render and presentation


    I'm skipping in-flight frames, and swap chain re-creation - this is just rebuild everything, not that
    interesting


    For vertex buffer
    Create binding and attribute descriptors
    Tell the graphics pipeline about it
    Create the vertex buffer
    Create the buffer memory
    Memcopy into the memory
    Bind the buffer in the command recording

    Optionally use a staging buffer, then copy to GPU-side only buffer for performance

    Uniform buffers (Used for transformation matrices)
    Works similarly as vertex
    Pipeline and command recording need to know about it
    copy data into a buffer

    A better way to pass this is using push constants
    Push constants allow faster transfer of small amounts of data without using a buffer
    They're constant from the shader's perspective
    They're pushed with the command buffer data
*/
