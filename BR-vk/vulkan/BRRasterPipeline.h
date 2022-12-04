#pragma once

#include <BRDevice.h>
#include <BRPipeline.h>
#include <BRRasterPipeline.h>
#include <BRRenderPass.h>
#include <BRSwapchain.h>

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class RasterPipeline : public Pipeline
{
   public:
    RasterPipeline(){};
    ~RasterPipeline(){};

    void addVertexInputInfo(
        vk::VertexInputBindingDescription& binding,
        std::vector<vk::VertexInputAttributeDescription>& attributes );
    void addInputAssembly( vk::PrimitiveTopology topology, bool restart );
    void addViewport( vk::Extent2D extent );
    void addRasterizer( vk::CullModeFlagBits cull, vk::FrontFace frontFace );
    void addDepthSencil( vk::CompareOp op );
    void addMultisampling( vk::SampleCountFlagBits samples );
    void addColorBlend();
    void addDynamicStates( std::vector<vk::DynamicState> states );

    void build( std::string name, RenderPass& renderpass,
                vk::DescriptorSetLayout layout );

   private:
    vk::PipelineVertexInputStateCreateInfo m_vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo m_inputAssembly;
    vk::PipelineViewportStateCreateInfo m_viewportState;
    vk::PipelineRasterizationStateCreateInfo m_rasterizer;
    vk::PipelineDepthStencilStateCreateInfo m_depthStencil;
    vk::PipelineMultisampleStateCreateInfo m_multisampling;
    vk::PipelineColorBlendAttachmentState m_colorBlendAttachment;
    vk::PipelineColorBlendStateCreateInfo m_colorBlend;
    std::vector<vk::DynamicState> m_dynamicStates;
};
}  // namespace BR
