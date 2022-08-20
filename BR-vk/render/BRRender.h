#pragma once

#define GLFW_INCLUDE_VULKAN

#include <BRAppState.h>
#include <BRBufferAllocator.h>
#include <BRCommandPool.h>
#include <BRDevice.h>
#include <BRFramebuffer.h>
#include <BRInstance.h>
#include <BRPipeline.h>
#include <BRRenderPass.h>
#include <BRSurface.h>
#include <BRSwapchain.h>
#include <BRSyncMgr.h>
#include <BRDescMgr.h>
#include <BRASBuilder.h>
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
    void run();
    bool m_framebufferResized = false;

   private:
    GLFWwindow* m_window;

    RenderPass m_renderPass;
    Pipeline m_pipeline;

    Framebuffer m_framebuffer;
    CommandPool m_commandPool;
    SyncMgr m_syncMgr;
    BufferAllocator m_bufferAlloc;
    DescMgr m_descMgr;
    ASBuilder m_asBuilder;

    vk::Device m_device;

    std::vector<vk::CommandBuffer> m_commandBuffers;
    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;

    vk::Buffer m_vertexBuffer;
    vk::Buffer m_indexBuffer;

    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;
    std::vector<vk::Buffer> m_uniformBuffers;

    int m_currentFrame = 0;

    std::vector<Pipeline::Vertex> m_vertices;
    std::vector<uint16_t> m_indices;

    vk::AccelerationStructureKHR m_blas;
    vk::AccelerationStructureKHR m_tlas;

    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };


    void initWindow();
    void initVulkan();

    void loadModel();

    void recreateSwapchain();

    void recordCommandBuffer( vk::CommandBuffer commandBuffer,
                              uint32_t imageIndex );
    void mainLoop();
    void drawFrame();
    void cleanup();

    void takeScreenshot();

    void initUI();
    void createBLAS();
    void drawUI();
    uint64_t getBufferDeviceAddress( VkBuffer buffer );

    void createDescriptorSets();
    void updateUniformBuffer( uint32_t currentImage );
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
