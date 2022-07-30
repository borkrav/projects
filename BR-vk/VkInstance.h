#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include "vulkan/BRInstance.h"
#include "vulkan/BRDevice.h"
#include "vulkan/BRSurface.h"


class VulkanEngine
{
   public:
    void run();

   private:
    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;

        // This defines how to read the data
        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof( Vertex );
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        // This describes where the vertex is and where the color is
        // type of color, type of vertex
        // location matches the location inside the shader
        static std::array<VkVertexInputAttributeDescription, 2>
        getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 2>
                attributeDescriptions{};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof( Vertex, pos );
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof( Vertex, color );
            return attributeDescriptions;
        }
    };

    GLFWwindow* m_window;

    BR::Instance m_instance;
    BR::Device m_device;
    BR::Surface m_surface;

    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkCommandPool m_commandPool;
    VkCommandBuffer m_commandBuffer;
    VkSemaphore m_imageAvailableSemaphore;
    VkSemaphore m_renderFinishedSemaphore;
    VkFence m_inFlightFence;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;

    const std::vector<Vertex> m_vertices = {
        { { 0.0f, -0.5f }, { 1.0f, 1.0f, 1.0f } },
        { { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } } };

    // int m_graphicsQueueFamilyIndex = 0;

    void initWindow();
    void initVulkan();
    VkShaderModule createShaderModule( const std::vector<char>& code );
    void createInstance();
    void setupGPU();
    void createSurface();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createVertexBuffer();
    void createCommandBuffer();
    void recordCommandBuffer( VkCommandBuffer commandBuffer,
                              uint32_t imageIndex );
    void createSyncObjects();
    void mainLoop();
    void drawFrame();
    void cleanup();
    uint32_t findMemoryType( uint32_t typeFilter,
                             VkMemoryPropertyFlags properties );
};
