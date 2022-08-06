#pragma once

#include <BRDevice.h>
#include <BRRenderPass.h>
#include <BRSwapchain.h>

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class Pipeline
{
   public:
    Pipeline();
    ~Pipeline();

    void create( RenderPass& renderpass, std::string name );
    void destroy();
    vk::Pipeline get();

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;

        // This defines how to read the data
        static vk::VertexInputBindingDescription getBindingDescription()
        {
            vk::VertexInputBindingDescription bindingDescription = {};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof( Vertex );
            bindingDescription.inputRate = vk::VertexInputRate::eVertex;

            return bindingDescription;
        }

        // This describes where the vertex is and where the color is
        // type of color, type of vertex
        // location matches the location inside the shader
        static std::array<vk::VertexInputAttributeDescription, 2>
        getAttributeDescriptions()
        {
            std::array<vk::VertexInputAttributeDescription, 2>
                attributeDescriptions = {};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
            attributeDescriptions[0].offset = offsetof( Vertex, pos );

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
            attributeDescriptions[1].offset = offsetof( Vertex, color );

            return attributeDescriptions;
        }
    };

   private:
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;
    vk::Device m_device;
};
}  // namespace BR
