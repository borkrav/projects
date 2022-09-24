#pragma once

#include <BRDevice.h>
#include <BRRenderPass.h>
#include <BRSwapchain.h>

#include <vector>
#include <vulkan/vulkan_handles.hpp>

namespace BR
{
class Framebuffer
{
   public:
    Framebuffer();
    ~Framebuffer();

    void create( std::string name, RenderPass& renderpass );
    void destroy();
    std::vector<vk::Framebuffer>& get();

   private:
    std::vector<vk::Framebuffer> m_swapChainFramebuffers;
    vk::Device m_device;
};
}  // namespace BR
