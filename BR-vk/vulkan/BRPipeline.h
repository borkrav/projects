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

    void create( std::string name, RenderPass& renderpass, vk::DescriptorSetLayout layout );
    void destroy();
    vk::Pipeline get();
    vk::PipelineLayout getLayout();

    struct Vertex
    {
        glm::vec3 pos;

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
        static std::array<vk::VertexInputAttributeDescription, 1>
        getAttributeDescriptions()
        {
            std::array<vk::VertexInputAttributeDescription, 1>
                attributeDescriptions = {};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[0].offset = offsetof( Vertex, pos );

            return attributeDescriptions;
        }
    };

   private:
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_graphicsPipeline;
    vk::Device m_device;
};
}  // namespace BR
