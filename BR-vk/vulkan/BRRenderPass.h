#pragma once

#include <BRDevice.h>
#include <BRSwapchain.h>

#include <vector>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class RenderPass
{
   public:
    RenderPass();
    ~RenderPass();

    void destroy();
    vk::RenderPass get();

    void addAttachment( vk::Format format, vk::AttachmentLoadOp load,
                        vk::AttachmentStoreOp store, vk::ImageLayout initial,
                        vk::ImageLayout final, vk::ImageLayout ref );

    void addSubpass( vk::PipelineBindPoint bind, int color, int depth );

    void addDependency( uint32_t src, uint32_t dest,
                        vk::PipelineStageFlags srcStageMask,
                        vk::PipelineStageFlags dstStageMask,
                        vk::AccessFlags srcAccessMask,
                        vk::AccessFlags dstAccessMask );

    void build( std::string name );

   private:
    vk::RenderPass m_renderPass;
    vk::Device m_device;

    std::vector<vk::AttachmentDescription> m_attachments;
    std::vector<vk::AttachmentReference> m_attachmentRefs;
    std::vector<vk::SubpassDescription> m_subpasses;
    std::vector<vk::SubpassDependency> m_dependencies;
};
}  // namespace BR
