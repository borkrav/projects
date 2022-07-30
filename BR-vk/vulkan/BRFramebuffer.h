#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include <BRDevice.h>
#include <BRRenderPass.h>
#include <BRSwapchain.h>

namespace BR
{
class Framebuffer
{
   public:
    Framebuffer();
    ~Framebuffer();

    void create( Swapchain& swapchain, RenderPass& renderpass, Device& device );
    void destroy( Device& device );
    std::vector<VkFramebuffer>& get();

   private:
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
};
}  // namespace BR
