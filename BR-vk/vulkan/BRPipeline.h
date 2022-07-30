#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>

#include <BRDevice.h>
#include <BRRenderPass.h>
#include <BRSwapchain.h>

namespace BR
{
class Pipeline
{
   public:
    Pipeline();
    ~Pipeline();

    void create( Device& device, Swapchain& swapchain, RenderPass& renderpass );
    void destroy( Device& device );
    VkPipeline get();

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

   private:
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
};
}  // namespace BR
